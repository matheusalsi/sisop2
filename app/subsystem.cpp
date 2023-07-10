#include "subsystems.h"

WOLSubsystem::WOLSubsystem(bool manager){
    this->manager = manager;
}

// O subsistema só pode ser destruído APÓS ter parado!
WOLSubsystem::~WOLSubsystem(){
    // Lançar exceção aqui?
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
    runThread = new std::thread(&WOLSubsystem::run, this); // Apesar de chamarmos o método de "WOLSubsystem", o polimorfismo de start é considerado
}

void WOLSubsystem::stop(){
    setRunning(false);
    if(runThread != NULL){
        runThread->join();
        delete runThread;
    }

}