#ifndef __MANAGEMENT_H__
#define __MANAGEMENT_H__

#include "subsystems.h"

// Subsistema de gerenciamento
class ManagementSS : public WOLSubsystem{

    public:
    void run();

    ManagementSS(bool isManager) : WOLSubsystem(isManager) {}

};

#endif // __MANAGEMENT_H__
