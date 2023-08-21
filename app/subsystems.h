#ifndef SUBSYSTEMS_H
#define SUBSYSTEMS_H

//define DEBUG

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
#include <errno.h>

#include "socket.h"
#include "packet.h"
#include "management.h"
#include "globals.h"

// Subsistema genérico do WakeOnLan, que roda numa thread própria
class WOLSubsystem{

    protected:
    void setRunning(bool value);


    private:
    bool manager; // define se o subsistema atua no modo gerente
    bool running; // define se o SS está rodando. Caso não esteja, threads devem parar de rodar
    // void setRunning(bool value);
    
    void run(); // Chamada por start, thread principal do subsistema


    protected:
    
    std::thread* runThread; // thread principal do SS
    
    // Gerenciador da tabela, com o qual o subsistema se comunica para escritas/leituras
    TableManager* tableManager;
    
    public:
    WOLSubsystem(bool isManager, TableManager* tableManager) : manager(isManager), tableManager(tableManager) {};
    ~WOLSubsystem();
    bool isManager();
    bool isRunning();
    
    virtual void start(); // Inicializa execução do subsistema
    virtual void stop(); // Encerra execução do subsistema

    virtual void runAsManager() = 0;
    virtual void runAsClient() = 0; // Execução de run() como cliente 

    void setManagerStatus(bool setAsManager);
};

#endif