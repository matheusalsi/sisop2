#include "subsystems.h"

// O subsistema só pode ser destruído APÓS ter parado!
// Isso porque senão a thread vai tentar rodar sobre um objeto já destruído.
// Inclusive isso causa erro de função puramente virtual: com o objeto destruído,
// vai ser chamado o run() de WOLSubsystem
WOLSubsystem::~WOLSubsystem(){
    // Lançar exceção caso sistema não tenha sido parado aqui?
}

bool WOLSubsystem::isManager(){
    return manager;
}

bool WOLSubsystem::isRunning(){
    return running;
}

void WOLSubsystem::setRunning(bool value){
    running = value;
}

void WOLSubsystem::start(){
    setRunning(true);
    runThread = new std::thread(&WOLSubsystem::run, this); // Apesar de chamarmos o método de "WOLSubsystem", o polimorfismo de run é considerado
}

void WOLSubsystem::stop(){
    setRunning(false);
    if(runThread != NULL){
        runThread->join();
        delete runThread;
    }

}

void WOLSubsystem::run(){
    while(isRunning()){
        if(g_electionHappening){
            return;
        }
        
        if(isManager()){
            runAsManager();
        }
        else{
            runAsClient();
        }
    }
}

void WOLSubsystem::setManagerStatus(bool isManager){
    manager = isManager;
}