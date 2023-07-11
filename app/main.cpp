#include "interface.h"
#include "discovery.h"

#include <string>
#include <signal.h>


bool stopExecution = false;

void handle_sigint(int signum){
    stopExecution = true;
}

bool isManager(int argc){
    return argc == 2;
}

int main(int argc, char *argv[])
{
    bool manager = isManager(argc);
    // Handling de Ctrl+c
    signal(SIGINT, handle_sigint);

    if(manager){
        std::cout << "Eu sou o gerente!" << std::endl;
    }
    else{
        std::cout << "Eu sou um participante" << std::endl;
    }

    DiscoverySS discoverySS(manager);
    discoverySS.start();

    // InterfaceSS interfaceSS(manager);
    // interfaceSS.start();

    while(!stopExecution){

    }
    discoverySS.stop();
    //interfaceSS.stop();
    std::cout << "FINALIZADO!" << std::endl;

    return 0;
}