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



    #ifdef DEBUG
    if(manager)
        std::clog << "Eu sou o gerente!" << std::endl;
    else
        std::clog << "Eu sou um participante" << std::endl;
    #endif

    TableManager tableManager;

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