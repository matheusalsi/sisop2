#ifndef DISCOVERY_H
#define DISCOVERY_H

#include <fstream>
#include "subsystems.h"

#define DISCOVERY_PORT 7575

// Subsistema de descoberta
class DiscoverySS : public WOLSubsystem{

    private:

    Socket discoverySocket; // Socket para envio/recepção de packets "Sleep Service Discovery"

    std::string hostname;
    std::string macaddress;

    char* managerIp;

    // Específicos para modo "participante" (client)
    bool foundManager = false;

    // Thread que envia pacotes "Sleep Service Discovery" enquanto não há resposta do manager
    void sendSleepDiscoverPackets();
    void sendSleepExitPackets(struct sockaddr_in serverAddrIn);

    public:
    void start();
    void stop();
    void run();

    std::string getHostname();
    std::string getMACAddress();

    DiscoverySS(bool isManager, TableManager* tableManager) : 
        WOLSubsystem(isManager, tableManager),
        discoverySocket(DISCOVERY_PORT),
        hostname(""),
        macaddress("")
    {};
};

#endif