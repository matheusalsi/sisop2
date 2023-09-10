#include "interface.h"
#include "management.h"
#include "discovery.h"
#include "monitoring.h"

#include <signal.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include "globals.h"


bool g_exiting = false;
// Controla se há uma eleição ocorrendo
bool g_electionHappening = false;
// Se um manager já foi encontrado, então esse processo
// passa a se atentar a eleições
bool g_foundManager = false;
// Variável utilizada para controlar caso o manager antigo acorde
bool g_becomeParticipant = false;

// Logger mostrado na interface
Logger logger;

std::string g_myIP;

// Controla se algum candidato com ID maior respondeu a eleição
bool electionAnswered;
// Controla se o novo manager mandou a mensagem avisando que ele foi eleito 
bool newManagerMessaged;

void handleSigint(int signum){
    g_exiting = true;
}

bool isManager(int argc){
    return argc == 2;
}

bool checkIPGreaterThan(std::string ip1, std::string ip2){
    return std::stoi(ip1.substr(ip1.find_last_of('.')+1)) > std::stoi(ip2.substr(ip2.find_last_of('.')+1));
}


#define ELECTION_PORT 27578

// Thread que responde a eleições.
void electionAcknowledgerThread(TableManager& tableManager){
    Socket electionSocket(ELECTION_PORT);
    electionSocket.openSocket();
    electionSocket.setSocketTimeoutMS(100);
    electionSocket.bindSocket();

    // Pacote de eleição
    packet packet;
    uint16_t clientPort;
    std::string clientIpStr;

    // Para somente com exit
    while(!g_exiting){
        // Recebemos um pacote de eleição. Checamos se é uma
        // resposta à nossa eleição ou uma notificação
        if(electionSocket.receivePacket(packet, clientPort, clientIpStr)){
            // Fomos avisados de eleição
            if(packet.type == (ELECTION_HAPPENING)){
                // Retorna pacote com ack
                logger.log("Respondendo à eleição");
                packet.type = (ELECTION_HAPPENING | ACKNOWLEDGE);
                electionSocket.sendPacket(packet, DIRECT_TO_IP, ELECTION_PORT, &clientIpStr);
                // Entra em modo de eleição se já não está
                g_electionHappening = true;
            }
            // Resposta de nossas notificações enviadas da thread principal
            else if(packet.type == (ELECTION_HAPPENING | ACKNOWLEDGE)){
                logger.log("Recebi resposta de um candidato");
                electionAnswered = true;
            }
            else if(packet.type == NEW_MANAGER){
                tableManager.setManagerIP(clientIpStr);
                newManagerMessaged = true;
                logger.log(std::string("Meu novo manager é - ") + clientIpStr);
            }

        }
    }

}



int main(int argc, char *argv[])
{
    // Handling de Ctrl+c
    signal(SIGINT, handleSigint);

    // Timestamp
    auto end = std::chrono::system_clock::now();
    std::time_t end_time = std::chrono::system_clock::to_time_t(end);
    
    std::clog << std::ctime(&end_time) << std::endl;

    // Descobre próprio IP
    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    struct ifreq ifr{};
    strcpy(ifr.ifr_name, "eth0");
    ioctl(fd, SIOCGIFADDR, &ifr);
    close(fd);
    char ip[INET_ADDRSTRLEN];
    strcpy(ip, inet_ntoa(((sockaddr_in*) &ifr.ifr_addr)->sin_addr));

    g_myIP = ip;

    #ifdef DEBUG
    if(manager)
        std::clog << "Eu sou o gerente!" << std::endl;
    else
        std::clog << "Eu sou um participante" << std::endl;
    #endif

    TableManager tableManager;

    DiscoverySS discoverySS(false, &tableManager);
    MonitoringSS monitoringSS(false, &tableManager);
    InterfaceSS interfaceSS(false, &tableManager);

    std::thread electionThread(&electionAcknowledgerThread, std::ref(tableManager));
    discoverySS.start();
    monitoringSS.start();
    interfaceSS.start();

    // Socket de envio de eleição
    Socket electionSenderSocket(ELECTION_PORT);
    electionSenderSocket.openSocket();
    
    // Pacote de eleição
    packet electionPacket;
    electionPacket.type = ELECTION_HAPPENING;

    // Espera por saída
    while(!g_exiting){
        // Manager recebeu um aviso de que ele deve virar participante 
        if (g_becomeParticipant){
            tableManager.setBackupStatus(true);
            discoverySS.setManagerStatus(false);
            monitoringSS.setManagerStatus(false);
            interfaceSS.setManagerStatus(false);
            
            g_becomeParticipant = false;
            g_electionHappening = false;
            g_foundManager = true;
        }
        // Se uma eleição está ocorrendo, envia pacotes para todos
        // PCs com ID maior e espera por respostas.
        // A thread de eleições irá notificar-nos por meio de uma v.global
        if(g_electionHappening){

            electionAnswered = false;
            newManagerMessaged = false;

            for(auto ip : *(tableManager.getKnownIps())){
                if(ip == g_myIP ) continue; // Não manda para si mesmo
                if(checkIPGreaterThan(g_myIP, ip)) continue; // Apenas manda para ips maiores (critério)
                logger.log(std::string("ELEIÇÃO: Enviando para ") + ip);
                
                electionSenderSocket.sendPacket(electionPacket, DIRECT_TO_IP, ELECTION_PORT, &ip); 
            }
            logger.log("Pacotes de eleição enviados para todos IDs maiores. Esperando por respostas (2.0 segundos)");

            // Após envio, espera um tempo para todas mensagens serem processadas
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));

            bool runAsManager;

            // Checa se a thread servidora de eleições recebeu uma resposta
            if(electionAnswered){
                logger.log("Recebi resposta de 1 ou mais candidatos. Esperando a resposta do manager novo");
                // Se torna backup
                tableManager.setBackupStatus(true);
                runAsManager = false;
                // Espera a mensagem do manager 
                while (!newManagerMessaged){
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }
            else{
                logger.log("Não recebi nenhuma resposta, logo me proclamarei manager");
                // Deixa de ser backup
                tableManager.setBackupStatus(false);

                // Muda para manager
                runAsManager = true;

                logger.log("Me tornei o novo manager");

                // Envia mensagem para os participantes informando que ele é o novo manager
                packet newManagerPacket;

                newManagerPacket.type = NEW_MANAGER;
                for(auto ip : *(tableManager.getKnownIps())){
                if(ip == g_myIP) continue; // Não manda para si mesmo
                logger.log(std::string("Enviando para os participantes que eu sou o novo manager - ") + ip);
                
                electionSenderSocket.sendPacket(newManagerPacket, DIRECT_TO_IP, ELECTION_PORT, &ip); 
            }

            }
            
            discoverySS.setManagerStatus(runAsManager);
            monitoringSS.setManagerStatus(runAsManager);
            interfaceSS.setManagerStatus(runAsManager);

            g_electionHappening = false;
        }
        
    }

    std::cout << "Encerrando thread principal..." << std::endl;

    discoverySS.stop();
    std::cout << "Subsistema DISCOVERY encerrado..." << std::endl;
    monitoringSS.stop();
    std::cout << "Subsistema MONITORING encerrado..." << std::endl;
    interfaceSS.stop();
    std::cout << "Subsistema INTERFACE encerrado..." << std::endl;
    
    electionThread.join();

    std::cout << "----- FINALIZADO! -----" << std::endl;

    return 0;
}