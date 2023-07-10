#include "interface.h"
#include <string>
#include <signal.h>


bool stopExecution = false;

void handle_sigint(int signum){
    stopExecution = true;
}

int main(int argc, char *argv[])
{

    // Handling de Ctrl+c
    signal(SIGINT, handle_sigint);

    InterfaceSS interfaceSS(false);
    interfaceSS.start();

    while(!stopExecution){

    }

    interfaceSS.stop();
    std::cout << "FINALIZADO!" << std::endl;

    return 0;
}