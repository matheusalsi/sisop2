#ifndef __MANAGEMENT_H__
#define __MANAGEMENT_H__

#include "subsystems.h"
#include <map>
#include <string>
#include <iomanip>
#include <signal.h>

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

// Subsistema de gerenciamento
class ManagementSS : public WOLSubsystem{
    private:
    WOLTable table;
    void setMailBoxVec(std::vector<std::string>& vectorMailBox, std::string messageMailBox);

    public:
    void start();
    // void stop();
    void run();

    ManagementSS(bool isManager) : WOLSubsystem(isManager) {}

};

#endif // __MANAGEMENT_H__
