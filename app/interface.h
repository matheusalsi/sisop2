#ifndef __INTERFACE_H__
#define __INTERFACE_H__



#include "subsystems.h"
#include <map>
#include <string>
#include <iomanip>
#include <condition_variable>
#include <regex>
#include <pthread.h>

#include "globals.h"

// Subsistema de interface
class InterfaceSS : public WOLSubsystem{

    private:
    
    // Lock para print; obtenção de input trava print da tabela
    std::mutex printLock;

    // Thread de input
    std::thread inputThread;
    void inputHandlerThread();
    // Realiza print
    std::thread printThread;
    void printInterfaceThread();
    
    

    // Handling de Ctrl+c
    static void handleSIGINT(int signum);
    bool waitingInput;

    // // Faz o exit
    // void handleExit();
    // bool exiting;

    public:
    InterfaceSS(bool isManager, TableManager* tableManager) : WOLSubsystem(isManager, tableManager) {};

    void start();
    void stop();
    void runAsManager();
    void runAsClient();
};

#endif // __INTERFACE_H__
