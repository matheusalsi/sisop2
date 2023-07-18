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
    bool addLine(std::string ipaddr, std::string hostname, std::string macaddr); //Adiciona linha contendo info desse ip
    bool removeLine(std::string ipaddr); // Remove linha contendo info desse ip
    void updateLineStatus(std::string ipaddr, std::string status); // Atualiza status desse ip
    std::string getMacFromHostname(std::string hostname); // Retorna o MAC address de um hostname (utilizado para fazer o WAKEONLAN pela interface)

};

// Subsistema de gerenciamento
class ManagementSS : public WOLSubsystem{
    private:
    WOLTable table;
   
    // Retorna em function o nome da função que foi passada para a mensagem, e em parameters os parâmetros que serão utilizados nessa função
    void getFunctionAndParametersFromMessage(std::string message, std::string& function, std::vector<std::string>& parameters);

    public:
    // void start();
    // void stop();
    void run();

    ManagementSS(bool isManager) : WOLSubsystem(isManager) {}

};

#endif // __MANAGEMENT_H__
