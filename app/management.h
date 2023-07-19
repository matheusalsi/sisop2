#ifndef __MANAGEMENT_H__
#define __MANAGEMENT_H__

#include "subsystems.h"
#include "tables.h"
#include <signal.h>
#include <set>



// Subsistema de gerenciamento
class ManagementSS : public WOLSubsystem{
    private:
    WOLTable table;
   
    // Indica se houveram atualizações na tablea
    std::set<std::string> recentlyUpdatedIPs;
   

    // Retorna em function o nome da função que foi passada para a mensagem, e em parameters os parâmetros que serão utilizados nessa função
    void getFunctionAndParametersFromMessage(std::string message, std::string& function, std::vector<std::string>& parameters);

    public:
    // void start();
    // void stop();
    void run();

    ManagementSS(bool isManager) : WOLSubsystem(isManager) {}

};

#endif // __MANAGEMENT_H__
