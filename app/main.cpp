#include "mailbox.h"
#include "interface.h"
#include "management.h"
#include "discovery.h"
#include "monitoring.h"

#include <signal.h>
#include <string.h>
#include <fstream>
#include <sys/ioctl.h>
#include <linux/if.h>


bool stopExecution = false;

void handleSigint(int signum){
    stopExecution = true;
}

bool isManager(int argc){
    return argc == 2;
}

int main(int argc, char *argv[])
{
    bool manager = isManager(argc);
    // Handling de Ctrl+c
    signal(SIGINT, handleSigint);

    if(manager){
        std::cout << "Eu sou o gerente!" << std::endl;
    }
    else{
        std::cout << "Eu sou um participante" << std::endl;
    }

    DiscoverySS discoverySS(manager);
    ManagementSS managementSS(manager);
    MonitoringSS monirotingSS(manager);
    InterfaceSS interfaceSS(manager);

    // Writer: Discovery - Reader: Management
    connectMailboxes(managementSS.getMailbox(), "M_IN", discoverySS.getMailbox(), "D_OUT");

    // Writer: Discovery - Reader: Monitoring
    connectMailboxes(monirotingSS.getMailbox(), "MO_IN", discoverySS.getMailbox(), "D2_OUT");

    // Writer: Interface - Reader: Discovery
    connectMailboxes(discoverySS.getMailbox(), "D_IN", interfaceSS.getMailbox(), "I_OUT");


    // Obtém hostname
    std::string hostname; // String vazia significa que hostname não foi definido
    std::ifstream hostname_file;
    hostname_file.open("/etc/hostname");

    if(hostname_file.is_open()){
        getline(hostname_file, hostname); 
    }

    // Obtém MAC
    struct ifreq iface;
    std::string macaddr_str;

    int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if(sockfd >= 0){
        strcpy(iface.ifr_name, "eth0");

        if (ioctl(sockfd, SIOCGIFHWADDR, &iface) == 0) {
            char iface_str [18];
            sprintf(iface_str, "%02x:%02x:%02x:%02x:%02x:%02x",
                    (unsigned char) iface.ifr_addr.sa_data[0],
                    (unsigned char) iface.ifr_addr.sa_data[1],
                    (unsigned char) iface.ifr_addr.sa_data[2],
                    (unsigned char) iface.ifr_addr.sa_data[3],
                    (unsigned char) iface.ifr_addr.sa_data[4],
                    (unsigned char) iface.ifr_addr.sa_data[5]);
            macaddr_str = iface_str;
        }
        
    }
    
    if(hostname.empty()){
        std::cout << "Não foi possível obter um hostname" << std::endl;
    }
    else{
        discoverySS.setHostname(hostname);
    }

    if(macaddr_str.empty()){
        std::cout << "Não foi possível obter o endereço MAC" << std::endl;
    }
    else{
        discoverySS.setMACAddress(macaddr_str);
    }

    #ifdef DEBUG
    std::cout << "Meu hostname: "<< hostname << std::endl;
    std::cout << "Meu MAC: " << macaddr_str << std::endl;
    #endif


    discoverySS.start();
    managementSS.start();
    monirotingSS.start();
    interfaceSS.start();

    // Executa enquanto todos os subsistemas estão rodando
    while(!stopExecution && interfaceSS.isRunning() && discoverySS.isRunning() && managementSS.isRunning() && monirotingSS.isRunning()){

    }

    managementSS.stop();
    discoverySS.stop();
    monirotingSS.stop();
    interfaceSS.stop();
    std::cout << "FINALIZADO!" << std::endl;

    return 0;
}