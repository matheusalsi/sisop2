#include "mailbox.h"
#include "interface.h"
#include "management.h"
#include "discovery.h"
#include "monitoring.h"

#include <signal.h>
#include <string.h>
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