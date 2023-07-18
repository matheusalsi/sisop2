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

// Subsistema de monitoramento
class MonitoringSS : public WOLSubsystem {

    private:
    Socket monitoringSocket; // Socket para envio/recepção de packets "Sleep Status Request"

    std::set<std::string> ipList; // Conjunto de IPs para monitoramento
    
    // void setIpList(std::vector<std::string>& ipList, std::string messageClientsIps);
    // std::vector<std::string> getIplist();
    
    // Thread que envia e espera pacotes "Sleep Status Request" 
    void sendSleepStatusPackets(struct sockaddr_in managerAddrIn);

    public:
    void start();
    void stop();
    void run();

    MonitoringSS(bool isManager) :
        WOLSubsystem(isManager),
        monitoringSocket(MONITORING_PORT, false)
        // ipList({})
    {};

};

#endif