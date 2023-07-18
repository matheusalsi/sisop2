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
    connectMailboxes(managementSS.getMailbox(), "M_IN <- D_OUT", discoverySS.getMailbox(), "D_OUT -> M_IN");

    // Reader: Management - Writer: Monitoring
    connectMailboxes(managementSS.getMailbox(), "M_IN <- MO_OUT", monitoringSS.getMailbox(), "MO_OUT -> M_IN");

    // Reader: Management - Writer: Interface
    connectMailboxes(managementSS.getMailbox(), "M_IN <- I_OUT", interfaceSS.getMailbox(), "I_OUT -> M_IN");

    // Reader: Interface - Writer: Management
    connectMailboxes(interfaceSS.getMailbox(), "I_IN <- M_OUT", managementSS.getMailbox(), "M_OUT -> I_IN");

    // Reader: Interface - Writer: Discovery: 
    connectMailboxes(interfaceSS.getMailbox(), "I_IN <- D_OUT", discoverySS.getMailbox(), "D_OUT -> I_IN");

    // Reader: Monitoring - Writer: Management
    connectMailboxes(monitoringSS.getMailbox(), "MO_IN <- M_OUT", managementSS.getMailbox(), "M_OUT -> MO_IN");

    // Reader: Monitoring - Writer: Discovery (PARA DEBUG)
    connectMailboxes(monitoringSS.getMailbox(), "MO_IN <- D_OUT", discoverySS.getMailbox(), "D_OUT -> MO_IN");

    // Reader: Discovery - Writer: Interface 
    connectMailboxes(discoverySS.getMailbox(), "D_IN <- I_OUT", interfaceSS.getMailbox(), "I_OUT -> D_IN");
    
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
    while(!stopExecution && discoverySS.isRunning() && managementSS.isRunning() && monitoringSS.isRunning() && interfaceSS.isRunning()){

    }

    discoverySS.stop();
    managementSS.stop();
    monitoringSS.stop();
    interfaceSS.stop();
    std::cout << "FINALIZADO!" << std::endl;

    return 0;
}