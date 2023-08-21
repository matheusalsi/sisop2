#ifndef MONITORING_H
#define MONITORING_H

#include "subsystems.h"
#include <vector>
#include <string>
#include <chrono>
#include <atomic>
#include <set>

#define MONITORING_PORT 7576
#define IPADDRESS_ROW_WIDTH 15
#define MONITORING_TIMEOUT_MS 100
#define MAX_MANAGER_MONITORING_TIMEOUTS 100

// Subsistema de monitoramento
class MonitoringSS : public WOLSubsystem {

    private:
    Socket monitoringSocket; // Socket para envio/recepção de packets "Sleep Status Request"
    
    // void setIpList(std::vector<std::string>& ipList, std::string messageClientsIps);
    // std::vector<std::string> getIplist();
    
    // Thread que envia e espera pacotes "Sleep Status Request" 
    void sendSleepStatusPackets(std::string ipstr);

    // Mapa de IPs para seu atual estado; resetado a cada ciclo de monitoring,
    // e preenchido de acordo com respostas aos pacotes
    std::map<std::string, bool> awakeStatus;

    public:
    void start();
    void stop();
    void runAsClient();
    void runAsManager();

    MonitoringSS(bool isManager, TableManager* tableManager) :
        WOLSubsystem(isManager, tableManager),
        monitoringSocket(MONITORING_PORT)
        // ipList({})
    {};

};

#endif