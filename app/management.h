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
#include <thread>

#include "socket.h"
#include "globals.h"

#define MANAGEMENT_PORT 27577

//#define DEBUG_TABLE
#define LOG_BACKUP

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
    // O IP do manager
    std::string managerIP;

    // Lock de acesso à tabela principal
    std::mutex tableLock;

    // Checa se é um backup (ouve por atualizações da tabela do manager)
    bool runningBackup = false;

    // Socket que ouve e envia mensagens de backup
    Socket backupSocket;

    // Thread que ouve mensagens de atualização do backup
    std::thread* thrBackupListener;

    // Checa se o processo é manager
    bool isManager;

    public:


    TableManager(bool isManager);
    ~TableManager();

    // Define se está em estado de backup 
    void setBackupStatus(bool isBackup);


    // Thread que ouve mensagens de atualização da tabela do manager
    void backupListenerThread();

    // Envia pacote de backup
    bool sendBackupPacketToClients(uint8_t operation, std::string ip, IpInfo& ipInfo);

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
    // Obtém IP do gerenciador
    std::string getManagerIP();
    // Envia a tabela para um IP
    void sendTableToIP(std::string ip);
    // Define o ip manager
    void setManagerIP(std::string str);

};

#endif // __MANAGEMENT_H__
