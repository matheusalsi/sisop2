#include "management.h"

void managementSubsystemThread(){
    std::cout << "Eu sou o subsistema de gerenciamento!" << std::endl;
    
    while(true){
        std::cout << "2";
    }


}

void ManagementSS::run(){
    while(isRunning()){
        if(!mailBox.isEmpty("D_OUT")){
            std::string msg;
            mailBox.readMessage("D_OUT", msg);
            std::cout << "Mensagem de DISCOVERY: " << msg << std::endl; 
        }
    }
}

