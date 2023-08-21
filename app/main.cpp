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
// Logger mostrado na interface
Logger logger;

std::string g_myIP;

void handleSigint(int signum){
    g_exiting = true;
}

bool isManager(int argc){
    return argc == 2;
}

bool checkIPGreaterThan(std::string ip1, std::string ip2){
    return std::stoi(ip1.substr(ip1.find_last_of('.'))) > std::stoi(ip2.substr(ip2.find_last_of('.')));
}


#define ELECTION_PORT 27578

// Thread que responde a eleições.
void electionAcknowledgerThread(){
    Socket electionSocket(ELECTION_PORT);
    electionSocket.openSocket();
    electionSocket.setSocketTimeoutMS(100);
    electionSocket.bindSocket();

    // Pacote de eleição
    packet packet;
    uint16_t clientPort;
    std::string clientIpStr;

    // Para somente com exit enquanto não há eleição
    while(!g_exiting && !g_electionHappening){
        // Recebemos um pacote de eleição. Checamos se é uma
        // resposta à nossa eleição ou uma notificação
        if(electionSocket.receivePacket(packet, clientPort, clientIpStr)){
            // Fomos avisados de eleição
            if(packet.type == (ELECTION_HAPPENING)){
                // Retorna pacote com ack
                packet.type |= ACKNOWLEDGE;
                electionSocket.sendPacket(packet, DIRECT_TO_IP, clientPort, &clientIpStr);
                // Entra em modo de eleição se já não está
                g_electionHappening = true;
            }
            // Resposta de nossas notificações enviadas da thread principal
            else if(packet.type == (ELECTION_HAPPENING | ACKNOWLEDGE)){
                auto cur = std::chrono::system_clock::now();
                g_lastElectionResponse = std::chrono::system_clock::to_time_t(cur);
            }
        }
    }

}



int main(int argc, char *argv[])
{
    bool manager = isManager(argc);
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

    TableManager tableManager(manager);

    DiscoverySS discoverySS(manager, &tableManager);
    MonitoringSS monitoringSS(manager, &tableManager);
    InterfaceSS interfaceSS(manager, &tableManager);

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

        // Se uma eleição está ocorrendo, envia pacotes para todos
        // PCs com ID maior e espera por respostas.
        // A thread de eleições irá notificar-nos por meio de uma v.global
        if(g_electionHappening){

            for(auto ip : *(tableManager.getKnownIps())){
                if(ip == g_myIP) continue; // Não manda para si mesmo
                if(checkIPGreaterThan(g_myIP, ip)) continue; // Apenas manda para ips maiores (critério)
                
                electionSenderSocket.sendPacket(electionPacket, DIRECT_TO_IP, ELECTION_PORT, &ip); 
            }
            electionLogger.log("Pacotes de eleição enviados para todos IDs maiores. Esperando por respostas (0.5 segundos)");

            auto cur = std::chrono::system_clock::now();
            time_t packetSendEndTime = std::chrono::system_clock::to_time_t(cur);

            // Após envio, espera um tempo para todas mensagens serem processadas
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

            // Checa se a thread servidora de eleições recebeu uma resposta
            if(packetSendEndTime < g_lastElectionResponse){
                electionLogger.log("Recebi resposta de 1 ou mais candidatos. Voltando ao modo Discovery");
                g_foundManager = false;
            }
            else{
                electionLogger.log("Não recebi nenhuma resposta, logo me proclamarei manager");
                // Deixa de ser backup
                tableManager.setBackupStatus(false);

                // Muda para manager
                g_isManager = true;

                logger.log("Me tornei o novo manager");

            }

            g_electionHappening = false;

        }

    std::cout << "Encerrando thread principal..." << std::endl;

    discoverySS.stop();
    std::cout << "Subsistema DISCOVERY encerrado..." << std::endl;
    monitoringSS.stop();
    std::cout << "Subsistema MONITORING encerrado..." << std::endl;
    interfaceSS.stop();
    std::cout << "Subsistema INTERFACE encerrado..." << std::endl;

    std::cout << "----- FINALIZADO! -----" << std::endl;

    return 0;
}