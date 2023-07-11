#include "interface.h"
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

    InterfaceSS interfaceSS(manager);
    interfaceSS.start();

    while(!stopExecution){

    }

    interfaceSS.stop();
    std::cout << "FINALIZADO!" << std::endl;

    return 0;
}