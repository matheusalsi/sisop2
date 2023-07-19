#ifndef __TABLES_H__
#define __TABLES_H__
#include <iostream>
#include <string>
#include <map>
#include <iomanip>
#include <sstream>

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

    // Escreve valores em forma de mensagem
    void appendToMessageString(std::string& msg);

};

// Tabela contendo informações do WOL
class WOLTable{

    private:
    std::map<std::string, WOLTableLine> lines; // Mapa de endereço IP para linha da tabela contendo infos
    int hostname_max_len = HOSTNAME_ROW_WIDTH;
    
    // Hostnames para IPs
    std::map<std::string, std::string> hostnameMACMap;

    public:
    void printToConsole(); // Imprime a tabela na tela
    bool addLine(std::string ipaddr, std::string hostname, std::string macaddr, std::string status); //Adiciona linha contendo info desse ip
    bool removeLine(std::string ipaddr); // Remove linha contendo info desse ip
    bool checkLineStatusDiff(std::string ipaddr, std::string status); // Checa se novo status é diferente
    void updateLineStatus(std::string ipaddr, std::string status); // Atualiza status desse ip
    std::string getMacFromHostname(std::string hostname); // Retorna o MAC address de um hostname (utilizado para fazer o WAKEONLAN pela interface)
    void appendLineAsMessage(std::string ipaddr, std::string& msg); // Obtém uma linha em formato de mensagem;
    void updateLineFromMessage(std::string& msg); // Adiciona informações a uma linha através de mensagem
    bool hasIP(std::string &ipaddr); // Indica se o IP está na tabela
};

#endif // __TABLES_H__
