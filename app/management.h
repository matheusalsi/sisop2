#ifndef __MANAGEMENT_H__
#define __MANAGEMENT_H__

#include <signal.h>
#include <mutex>
#include <map>
#include <string>
#include <set>
#include <iostream>
#include <iomanip>
#include <sstream>
#include "globals.h"

//#define DEBUG_TABLE

#define HOSTNAME_ROW_WIDTH 20
#define MACADDRESS_ROW_WIDTH 17
#define IPADDRESS_ROW_WIDTH 15
#define STATUS_ROW_WIDTH 6

typedef struct IpInfo{
    std::string hostname;
    std::string mac;
    bool awake;
} IpInfo;


// Subsistema de gerenciamento
class TableManager{
    private:
    // Status de cada ip
    std::map<std::string, IpInfo> ipStatusTable;
    // Ips conhecidos (acesso rápido)
    std::set<std::string> knownIps;
    // Hostname->Mac, para wakeonlan
    std::map<std::string, std::string> macHostnameMap;


    // Lock de acesso à tabela principal
    std::mutex tableLock;

    public:
    // Adição e remoção de clientes à tabela (DISCOVERY)
    void insertClient(std::string ip, IpInfo ipInfo);
    void removeClient(std::string ip);
    // Atualização do estado de um cliente
    bool updateClient(bool awake, std::string ip);
    // Obtém IPs  
    const std::set<std::string>* getKnownIps();
    // Retorna tabela como string para print
    std::string getTablePrintString();
    // Obtém mac a partir de um hostname
    std::string getMacFromHostname(std::string hostname);

};

#endif // __MANAGEMENT_H__
