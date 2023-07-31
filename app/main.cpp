#include "interface.h"
#include "management.h"
#include "discovery.h"
#include "monitoring.h"

#include <signal.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/if.h>


bool g_stop_execution = false;

void handleSigint(int signum){
    g_stop_execution = true;
}

bool isManager(int argc){
    return argc == 2;
}

int main(int argc, char *argv[])
{
    bool manager = isManager(argc);
    // Handling de Ctrl+c
    signal(SIGINT, handleSigint);

    // Logging para arquivo
    std::ofstream log;
    if(manager){
        log.open("log_manager.txt");
    }
    else{
        log.open("log_participant.txt");
    }

    std::clog.rdbuf(log.rdbuf());

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

    // Somente a interface pode encerrar o programa
    while(!g_stop_execution && interfaceSS.isRunning()){

    }

    #ifdef DEBUG
    std::clog << "Encerrando a execução" << std:: endl;
    #endif


    discoverySS.stop();
    monitoringSS.stop();
    interfaceSS.stop();

    #ifdef DEBUG
    log.close();
    #endif

    std::cout << "FINALIZADO!" << std::endl;

    return 0;
}