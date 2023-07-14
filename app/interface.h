#ifndef __INTERFACE_H__
#define __INTERFACE_H__



#include "subsystems.h"
#include <map>
#include <string>
#include <iomanip>

#define HOSTNAME_ROW_WIDTH 8
#define MACADDRESS_ROW_WIDTH 17
#define IPADDRESS_ROW_WIDTH 15
#define STATUS_ROW_WIDTH 6

// Info de uma linha na tabela
struct WOLTableLine{
    std::string hostname = "N/A";
    std::string mac = "N/A";
    std::string ip = "N/A";
    std::string status = "N/A";

    WOLTableLine() :
    hostname("N/A"),
    mac("N/A"),
    ip("N/A"),
    status("N/A")
    {};

    WOLTableLine(std::string hostname, std::string mac, std::string ip, std::string status) :
    hostname(hostname),
    mac(mac),
    ip(ip),
    status(status)
    {};
};

// Tabela contendo informações do WOL
class WOLTable{

    private:
    std::map<std::string, WOLTableLine> lines; // Mapa de endereço IP para linha da tabela contendo infos
    int hostname_max_len = HOSTNAME_ROW_WIDTH;
    
    public:
    void printToConsole(); // Imprime a tabela na tela
    bool addLine(std::string ipaddr, std::string macaddr, std::string hostname); //Adiciona linha contendo info desse ip
    bool removeLine(std::string ipaddr); // Remove linha contendo info desse ip
    void updateLineStatus(std::string ipaddr, bool awake); // Atualiza status desse ip

};


// Subsistema de interface
class InterfaceSS : public WOLSubsystem{

    private:
    WOLTable table;
    void userInputInterface();

    public:
    
    InterfaceSS(bool isManager) : WOLSubsystem(isManager) {};

    void run();
    void printInterface();



};

#endif // __INTERFACE_H__
