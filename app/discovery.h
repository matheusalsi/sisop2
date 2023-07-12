#include "subsystems.h"
#define DISCOVERY_PORT 7575

// Subsistema de descoberta
class DiscoverySS : public WOLSubsystem{

    private:
    Socket socket; // Socket para envio/recepção de packets "Sleep Service Discovery"
    // Específicos para modo "participante" (client)
    bool foundManager = false;

    // Thread que envia pacotes "Sleep Service Discovery" enquanto não há resposta do manager
    void sendSleepDiscoverPackets();

    public:
    void start();
    void stop();
    void run();

    std::string getHostname();
    std::string getMACAddress();

    DiscoverySS(bool isManager) : WOLSubsystem(isManager), socket(DISCOVERY_PORT, true){};

};
