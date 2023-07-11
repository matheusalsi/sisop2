#include "subsystems.h"
#define DISCOVERY_PORT 7575

// Subsistema de descoberta
class DiscoverySS : public WOLSubsystem{

    private:
    int discoverySocketFD; // Socket para envio/recepção de packets "Sleep Service Discovery"
    struct sockaddr_in discoverySocketServerAddrIn; // Informações do servidor

    // Específicos para modo "participante" (client)
    bool foundManager = false;


    // Específicos para modo "manager" (server)
    struct sockaddr_in discoverySocketClientAddrIn; // Informações do cliente
    socklen_t cli_len; // Tamanho da informação do cliente

    // Thread que envia pacotes "Sleep Service Discovery" enquanto não há resposta do manager
    void sendSleepDiscoverPackets();

    public:
    void start();
    void stop();
    void run();

    std::string getHostname();
    std::string getMACAddress();

    DiscoverySS(bool isManager) : WOLSubsystem(isManager) {};


};
