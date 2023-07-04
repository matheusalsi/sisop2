#include <iostream>
#include <thread>
#include "subsystems.h"

int main(int argc, char *argv[])
{

    // Threads dos subsistemas
    std::thread th_discovery(discoverySubsystemThread);
    std::thread th_monitoring(monitoringSubsystemThread);
    std::thread th_management(managementSubsystemThread);
    std::thread th_interface(interfaceSubsystemThread);

    th_discovery.join();
    th_monitoring.join();
    th_management.join();
    th_interface.join();

    return 0;
}