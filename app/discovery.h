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

    // Específicos para modo "participante" (client)
    bool foundManager = false;
    bool hasLeft = false;

    // Thread que envia pacotes "Sleep Service Discovery" enquanto não há resposta do manager
    void sendSleepDiscoverPackets();
    void sendSleepExitPackets(struct sockaddr_in serverAddrIn);

    public:
    void start();
    void stop();
    void run();

    std::string getHostname();
    std::string getMACAddress();

    DiscoverySS(bool isManager) : 
        WOLSubsystem(isManager),
        discoverySocket(DISCOVERY_PORT, false),
        hostname(""),
        macaddress("")
    {};
};

#endif