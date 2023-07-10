#include "subsystems.h"

void interfaceSubsystemThread(){
    std::cout << "Eu sou o subsistema de interface!" << std::endl;

    while(true){
        std::cout << "4";
    }

}

void InterfaceSS::run(){
    while(isRunning()){
        std::cout << "Eu sou o SS da interface, rodando em paralelo!" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}