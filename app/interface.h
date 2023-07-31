#ifndef __INTERFACE_H__
#define __INTERFACE_H__



#include "subsystems.h"
#include <map>
#include <string>
#include <iomanip>
#include <condition_variable>
#include <regex>
#include "tables.h"
#include "globals.h"

// Subsistema de interface
class InterfaceSS : public WOLSubsystem{

    private:
    
    // Thread de input
    //void inputThread();

    void printInterface();
    
    // // Faz o exit
    // void handleExit();
    // bool exiting;

    public:
    InterfaceSS(bool isManager, TableManager* tableManager) : WOLSubsystem(isManager, tableManager) {};

    void stop();
    void run();



};

#endif // __INTERFACE_H__
