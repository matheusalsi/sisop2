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

int main(int argc, char *argv[])
{
    bool manager = isManager(argc);
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

    // Espera encerramento do subsistema de descoberta
    while(!g_exiting){

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