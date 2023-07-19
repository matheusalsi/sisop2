#ifndef __INTERFACE_H__
#define __INTERFACE_H__



#include "subsystems.h"
#include <map>
#include <string>
#include <iomanip>
#include <condition_variable>
#include "tables.h"

// Subsistema de interface
class InterfaceSS : public WOLSubsystem{

    private:
    // Tabela do subsistema para consistência entre prints
    WOLTable localTable;
    // Acesso à tabela
    std::mutex tableLock;
    bool hasTableUpdates;
    
    // Thread de print
    void printInterfaceThread();
    // Thread de input
    void inputThread();

    // Lida com mensagens de atualização da tabela (inserção, remoção, mudança de status)
    void handleUpdateMessage(std::string msg);

    bool exiting;

    public:
    InterfaceSS(bool isManager) : WOLSubsystem(isManager) {};

    void run();



};

#endif // __INTERFACE_H__
