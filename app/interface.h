#ifndef __INTERFACE_H__
#define __INTERFACE_H__



#include "subsystems.h"
#include <map>
#include <string>
#include <iomanip>
#include "tables.h"

// Subsistema de interface
class InterfaceSS : public WOLSubsystem{

    private:
    // Tabela do subsistema para consistência entre prints
    WOLTable localTable;

    public:
    InterfaceSS(bool isManager) : WOLSubsystem(isManager) {};

    void run();
    void printInterface();



};

#endif // __INTERFACE_H__
