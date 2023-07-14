#include "mailbox.h"
#include <string>

bool SubsystemMailBox::isEmpty(){
    bool r_val;
    bufferLock.lock();
    r_val = buffer.empty();
    bufferLock.unlock();
    return r_val;
}

void SubsystemMailBox::readMessage(std::string& msg){
    bufferLock.lock();
    msg = buffer.front(); // Copia para a string o valor na frente
    buffer.pop_front(); // Remove do buffer
    bufferLock.unlock();
}

void SubsystemMailBox::writeMessage(const char*  msg){
    bufferLock.lock();
    buffer.push_back(msg); // Adiciona string ao fim do buffer
    bufferLock.unlock();
}

void SubsystemMailBox::writeMessage(std::string&  msg){
    bufferLock.lock();
    buffer.push_back(msg); // Adiciona string ao fim do buffer
    bufferLock.unlock();
}