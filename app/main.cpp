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

void initMailboxes(DiscoverySS& discoverySS, ManagementSS& managementSS, InterfaceSS& interfaceSS, MonitoringSS& monitoringSS){
    // Reader: Management - Writer: Discovery
    connectMailboxes(managementSS.getMailbox(), "M_IN", discoverySS.getMailbox(), "D_OUT");

    // Reader: Management - Writer: Monitoring
    connectMailboxes(managementSS.getMailbox(), "M_IN2", monitoringSS.getMailbox(), "MO_OUT");

    // Reader: Monitoring - Writer: Discovery
    connectMailboxes(monitoringSS.getMailbox(), "MO_IN", discoverySS.getMailbox(), "D2_OUT");

    // Reader: Discovery - Writer: Interface 
    connectMailboxes(discoverySS.getMailbox(), "D_IN", interfaceSS.getMailbox(), "I_OUT");

    // Reader: Interface - Writer: Discovery: 
    connectMailboxes(interfaceSS.getMailbox(), "I_IN", discoverySS.getMailbox(), "D3_OUT");
}

std::string getHostName(){
    std::string hostname; // String vazia significa que hostname não foi definido
    std::ifstream hostname_file;
    hostname_file.open("/etc/hostname");

    if(hostname_file.is_open()){
        getline(hostname_file, hostname); 
    }
    return hostname;
}

std::string getMACAddress(){
    int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    std::string macaddr_str;
    ifreq iface;

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
    return macaddr_str;
}

void getComputerInfo(DiscoverySS& discoverySS){
    // Obtém hostname
    std::string hostname;
    hostname = getHostName();

    if(hostname.empty())
        std::cout << "Não foi possível obter um hostname" << std::endl;
    else
        discoverySS.setHostname(hostname);

    // Obtém MAC
    std::string macaddr;
    macaddr = getMACAddress();

     if(macaddr.empty())
        std::cout << "Não foi possível obter o endereço MAC" << std::endl;
    else
        discoverySS.setMACAddress(macaddr);

    #ifdef DEBUG
    std::cout << "Meu hostname: "<< hostname << std::endl;
    std::cout << "Meu MAC: " << macaddr << std::endl;
    #endif   
}



int main(int argc, char *argv[])
{
    bool manager = isManager(argc);
    // Handling de Ctrl+c
    signal(SIGINT, handleSigint);

    #ifdef DEBUG
    if(manager)
        std::cout << "Eu sou o gerente!" << std::endl;
    else
        std::cout << "Eu sou um participante" << std::endl;
    #endif

    DiscoverySS discoverySS(manager);
    ManagementSS managementSS(manager);
    MonitoringSS monitoringSS(manager);
    InterfaceSS interfaceSS(manager);

    initMailboxes(discoverySS, managementSS, interfaceSS, monitoringSS);

    discoverySS.start();
    managementSS.start();
    monitoringSS.start();
    interfaceSS.start();

    getComputerInfo(discoverySS);

    // Executa enquanto todos os subsistemas estão rodando
    while(!stopExecution && interfaceSS.isRunning() && discoverySS.isRunning() && managementSS.isRunning()/* && monitoringSS.isRunning()*/){

    }

    managementSS.stop();
    discoverySS.stop();
    monitoringSS.stop();
    interfaceSS.stop();
    std::cout << "FINALIZADO!" << std::endl;

    return 0;
}