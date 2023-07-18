#ifndef SUBSYSTEMS_H
#define SUBSYSTEMS_H

#define DEBUG


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


#include "socket.h"
#include "packet.h"
#include "mailbox.h"


// Subsistema genérico do WakeOnLan, que roda numa thread própria
class WOLSubsystem{

    protected:
    void setRunning(bool value);


    private:
    bool manager; // define se o subsistema atua no modo gerente
    std::thread* runThread; // thread principal do SS
    bool running; // define se o SS está rodando. Caso não esteja, threads devem parar de rodar
    // void setRunning(bool value);
    
    virtual void run()=0; // Chamada por start, thread principal do subsistema
    
    protected:
    SubsystemMailBox mailBox;

    public:
    WOLSubsystem(bool isManager);
    ~WOLSubsystem();
    bool isManager();
    bool isRunning();
    
    virtual void start(); // Inicializa execução do subsistema
    virtual void stop(); // Encerra execução do subsistema

    SubsystemMailBox& getMailbox();


};

#endif