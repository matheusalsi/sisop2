#include <iostream>

void discoverySubsystemThread();
void monitoringSubsystemThread();
void managementSubsystemThread();
void interfaceSubsystemThread();

// Subsistema genérico do WakeOnLan
class WOLSubsystem{
    private:
    bool isManager;

    public:
    WOLSubsystem(bool _isManager) : isManager(_isManager) {};
    
    virtual bool start() = 0; // Inicializa o subsistema
    virtual void stop() = 0; // Encerra o subsistema


};

// Subsistema de descoberta
class DiscoverySS : public WOLSubsystem{

};

// Subsistema de monitoramento
class MonitoringSS : public WOLSubsystem{

};

// Subsistema de gerenciamento
class ManagementSS : public WOLSubsystem{

};

// Subsistema de interface
class InterfaceSS : public WOLSubsystem{

};