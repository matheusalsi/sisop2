#include "subsystems.h"

// Subsistema de gerenciamento
class ManagementSS : public WOLSubsystem{

    public:
    SubsystemMailBox* discoverySSInbox;
    SubsystemMailBox* monitoringSSInbox;
    SubsystemMailBox* interfaceSSInbox;
    SubsystemMailBox* interfaceSSOutbox;
    
    void run();

    ManagementSS(bool isManager) : WOLSubsystem(isManager) {}

};