#include "management.h"

void managementSubsystemThread(){
    std::cout << "Eu sou o subsistema de gerenciamento!" << std::endl;
    
    while(true){
        std::cout << "2";
    }


}

void ManagementSS::run(){
    while(isRunning()){
        if(!discoverySSInbox->isEmpty()){
            std::string msg;
            discoverySSInbox->readMessage(msg);
            std::cout << "Mensagem de DISCOVERY: " << msg << std::endl; 
        }
    }
}

