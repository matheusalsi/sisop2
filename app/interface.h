#ifndef __INTERFACE_H__
#define __INTERFACE_H__



#include "subsystems.h"
#include <map>
#include <string>
#include <iomanip>
#include <signal.h>

// Subsistema de interface
class InterfaceSS : public WOLSubsystem{

    private:
    void userInputInterface();
    void handleKeyboardInterruption(int signum);

    public:
    
    InterfaceSS(bool isManager) : WOLSubsystem(isManager) {};

    void run();
    void printInterface();



};

#endif // __INTERFACE_H__
