#include "interface.h"
#include "management.h"
#include "discovery.h"
#include "monitoring.h"

#include <signal.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include "globals.h"

#define ELECTION_PORT 27578
#define MANAGER_SWITCH_PORT 27579


bool g_exiting = false;
// Controla se há uma eleição ocorrendo
bool g_electionHappening = false;
// Última resposta a uma eleição
time_t g_lastElectionResponse = 0;

// Se um manager já foi encontrado, então esse processo
// passa a se atentar a eleições por timeout no monitoramento
bool g_foundManager = false;

// Decide se o estado atual do processo é manager
bool g_isManager = false;

// Logger mostrado na interface
Logger logger;
// Logger para eleições
Logger electionLogger;

std::string g_myIP;
std::string g_myMACAddress;
std::string g_myHostname;

void handleSigint(int signum){
    g_exiting = true;
}

// Verdadeiro se ip1 > ip2
bool checkIPGreaterThan(std::string ip1, std::string ip2){
    return (std::stoi(ip1) > std::stoi(ip2));
}

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
    // Handling de Ctrl+c
    signal(SIGINT, handleSigint);

    // Timestamp
    auto end = std::chrono::system_clock::now();
    std::time_t end_time = std::chrono::system_clock::to_time_t(end);
    
    std::clog << std::ctime(&end_time) << std::endl;

    // Descobre próprio IP, MAC, hostname
    
    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    ifreq iface;
    strcpy(iface.ifr_name, "eth0");
    
    // IP
    ioctl(fd, SIOCGIFADDR, &iface);
    close(fd);
    char ip[INET_ADDRSTRLEN];
    strcpy(ip, inet_ntoa(((sockaddr_in*) &iface.ifr_addr)->sin_addr));
    g_myIP = ip;
    
    // MAC
    if (ioctl(fd, SIOCGIFHWADDR, &iface) == 0) {
        char iface_str [18];
        sprintf(iface_str, "%02x:%02x:%02x:%02x:%02x:%02x",
                (unsigned char) iface.ifr_addr.sa_data[0],
                (unsigned char) iface.ifr_addr.sa_data[1],
                (unsigned char) iface.ifr_addr.sa_data[2],
                (unsigned char) iface.ifr_addr.sa_data[3],
                (unsigned char) iface.ifr_addr.sa_data[4],
                (unsigned char) iface.ifr_addr.sa_data[5]);
        g_myMACAddress = iface_str;
    }

    // HOSTNAME

    std::ifstream hostname_file;
    hostname_file.open("/etc/hostname");

    if(hostname_file.is_open()){
        getline(hostname_file, g_myHostname); 
    }


    // Começa como cliente

    TableManager tableManager; // Backup
    DiscoverySS discoverySS(false, &tableManager);
    MonitoringSS monitoringSS(false, &tableManager);
    InterfaceSS interfaceSS(false, &tableManager);

    discoverySS.start();
    monitoringSS.start();
    interfaceSS.start();

    // Socket de envio de eleição
    Socket electionSenderSocket(ELECTION_PORT);
    electionSenderSocket.openSocket();
    // Socket que recebe novo manager
    Socket managerSwitchSocket(MANAGER_SWITCH_PORT);
    managerSwitchSocket.openSocket();
    managerSwitchSocket.bindSocket();

    // Pacote de eleição
    packet electionPacket;
    electionPacket.type = ELECTION_HAPPENING;

    // Pacote de novo manager
    packet newManagerPacket;
    uint16_t port;
    std::string newManagerIP;


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
                electionLogger.log("Recebi resposta de 1 ou mais candidatos. Esperando pelo novo manager.");
                // Espera pelo novo manager. Assumimos que ele não dorme nesse meio tempo
                // Também assumimos que qualquer packet recebido aqui é válido
                managerSwitchSocket.receivePacket(newManagerPacket, port, newManagerIP);
            }
            else{
                electionLogger.log("Não recebi nenhuma resposta, logo me proclamarei manager");
                // Manda um pacote qualquer
                for(auto ip : *(tableManager.getKnownIps())){
                    if(ip == g_myIP) continue; // Não manda para si mesmo
                    managerSwitchSocket.sendPacket(newManagerPacket, DIRECT_TO_IP, ELECTION_PORT, &ip);
                }
                electionLogger.log("Todos estão notificados que agora sou o manager");

                // Deixa de ser backup
                tableManager.setBackupStatus(false);

                // Muda para manager
                g_isManager = true;

                // Encerra eleição
                g_electionHappening = false;

                logger.log("Me tornei o novo manager");

            }

        }




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