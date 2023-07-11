#ifndef SUBSYSTEMS_H
#define SUBSYSTEMS_H


#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <strings.h>

#include "packet.h"

void discoverySubsystemThread();
void monitoringSubsystemThread();
void managementSubsystemThread();
void interfaceSubsystemThread();


// Subsistema genérico do WakeOnLan, que roda numa thread própria
class WOLSubsystem{

    private:
    bool manager; // define se o subsistema atua no modo gerente
    std::thread* runThread; // thread principal do SS

    bool running; // define se o SS está rodando. Caso não esteja, threads devem parar de rodar

    void setRunning(bool value);
    

    virtual void run()=0; // Chamada por start, thread principal do subsistema
    

    public:
    WOLSubsystem(bool isManager);
    ~WOLSubsystem();
    bool isManager();
    bool isRunning();
    
    virtual void start(); // Inicializa execução do subsistema
    virtual void stop(); // Encerra execução do subsistema


};

// Subsistema de monitoramento
class MonitoringSS : public WOLSubsystem{

};

// Subsistema de gerenciamento
class ManagementSS : public WOLSubsystem{

};

#endif