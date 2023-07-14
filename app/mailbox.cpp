#include "mailbox.h"

void connectMailboxes(SubsystemMailBox& in, const char* id1, SubsystemMailBox& out, const char* id2){
    std::mutex* lock =  new std::mutex;
    messageDeque* buffer = new messageDeque;
    in.bufferMap[std::string(id2)] = MailBoxBuffer(true, lock, buffer);
    out.bufferMap[std::string(id1)] = MailBoxBuffer(false, lock, buffer);
}


bool SubsystemMailBox::isEmpty(const char* bufferId){
    MailBoxBuffer& buffer = bufferMap[std::string(bufferId)];
    
    bool r_val;

    buffer.bufferLock.get()->lock();
    r_val = buffer.sharedBuffer.get()->empty();
    buffer.bufferLock.get()->unlock();
    
    return r_val;
}

void SubsystemMailBox::readMessage(const char* bufferId, std::string& msg){
    MailBoxBuffer& buffer = bufferMap[std::string(bufferId)];
    
    buffer.bufferLock.get()->lock();
    msg = buffer.sharedBuffer.get()->front(); // Copia para a string o valor na frente
    buffer.sharedBuffer.get()->pop_front(); // Remove mensagem do buffer
    buffer.bufferLock.get()->unlock();
    
}


void SubsystemMailBox::writeMessage(const char* bufferId, const char*  msg){
    MailBoxBuffer& buffer = bufferMap[std::string(bufferId)];
    
    buffer.bufferLock.get()->lock();
    buffer.sharedBuffer.get()->push_back(msg); // Copia para a string o valor na frente
    buffer.bufferLock.get()->unlock();
}

void SubsystemMailBox::writeMessage(const char* bufferId, std::string&  msg){
    MailBoxBuffer& buffer = bufferMap[std::string(bufferId)];
    
    buffer.bufferLock.get()->lock();
    buffer.sharedBuffer.get()->push_back(msg); // Copia para a string o valor na frente
    buffer.bufferLock.get()->unlock();
}