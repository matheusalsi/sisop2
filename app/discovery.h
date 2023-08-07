#ifndef DISCOVERY_H
#define DISCOVERY_H

#include <fstream>
#include "subsystems.h"

#define DISCOVERY_PORT 27575

// Subsistema de descoberta
class DiscoverySS : public WOLSubsystem{

    private:

    Socket discoverySocket; // Socket para envio/recepção de packets "Sleep Service Discovery"

    std::string hostname;
    std::string macaddress;

    // Específicos para modo "participante" (client)
    bool foundManager = false;

    // Ip do manager
    std::string managerIpStr;

    // Thread que envia pacotes "Sleep Service Discovery" enquanto não há resposta do manager
    void sendSleepDiscoverPackets();
    void sendSleepExitPackets();
    // Adiciona cliente e manager à tabela
    void prepareAndSendToTable(std::string macAndHostname, std::string ipStr);

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