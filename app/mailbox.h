#ifndef MAILBOX_H
#define MAILBOX_H

#include <mutex>
#include <deque>
#include <map>
#include <memory>
#include <string>

/* 
Exemplos:

DiscoverySS -> "127.0.0.1&Hostname&00:00:00:00:00:00" -> ManagerSS

*/


typedef std::deque<std::string> messageDeque;

// Um buffer da caixa postal
struct MailBoxBuffer{

    bool input; // Define se esse buffer é tratado como input ou output por seu dono
    std::shared_ptr<std::mutex> bufferLock; // Lock do buffer compartilhado
    std::shared_ptr<messageDeque> sharedBuffer; // Buffer compartilhado entre dois SS

    MailBoxBuffer() {};
    MailBoxBuffer(bool input, std::mutex* lock, messageDeque* buffer) :
    input(input),
    bufferLock(lock),
    sharedBuffer(buffer)
    {};

}; 

class SubsystemMailBox{

    private:
    // Mapeia uma string para um buffer.
    std::map<std::string, MailBoxBuffer> bufferMap;
    
    public:

    friend void connectMailboxes(SubsystemMailBox& in, const char* id1, SubsystemMailBox& out, const char* id2);


    // Retorna se um buffer está vazia
    bool isEmpty(const char* bufferId);
    // Lê uma mensagem de um buffer
    void readMessage(const char* bufferId, std::string&msg);
    // Escreve uma mensagem em um buffer
    void writeMessage(const char* bufferId, const char* msg );
    void writeMessage(const char* bufferId, std::string& msg );

};

void connectMailboxes(SubsystemMailBox& in, const char* id1, SubsystemMailBox& out, const char* id2);

#endif