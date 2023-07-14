#ifndef MAILBOX_H

#include <mutex>
#include <deque>

/* 
Exemplos:

DiscoverySS -> "127.0.0.1&Hostname&00:00:00:00:00:00" -> ManagerSS

*/

class SubsystemMailBox{

    std::mutex bufferLock;
    std::deque<std::string > buffer;

    public:

    // Retorna se a caixa está vazia
    bool isEmpty();
    // Lê uma mensagem da caixa
    void readMessage(std::string& str);
    // Escreve uma mensagem na caixa
    void writeMessage(const char* str );
    void writeMessage(std::string& str );
    
};



#endif