#include "monitoring.h"

void MonitoringSS::start(){
    monitoringSocket.openSocket();
     
    monitoringSocket.setSocketTimeoutMS(100);

    if(!isManager()){ // Participante (Servidor)
        monitoringSocket.bindSocket();
    }
    WOLSubsystem::start();
};

void MonitoringSS::stop(){
    WOLSubsystem::stop();
    monitoringSocket.closeSocket();
};

void MonitoringSS::run(){
    while (isRunning()) {
        if(isManager()){ // Cliente - envia os pacotes de sleep status requests
            
            // Obtém IPs da tabela
            const auto ipList = tableManager->getKnownIps();
            
            if (ipList->size() == 0) {
                #ifdef DEBUG
                std::clog << "MONITORING: ";
                std::clog << "Não há clientes para monitorar" << std::endl;
                #endif
                std::this_thread::sleep_for(std::chrono::seconds(1));
                continue;
            }

            #ifdef DEBUG
            std::clog << "MONITORING: ";
            std::clog << "Estou iniciando threads em " << MONITORING_PORT << std::endl;
            #endif  
            

            std::vector<std::thread> packetSenderThreadStatus;

            
            for(auto ip: *ipList) {
                #ifdef DEBUG
                std::clog << "MONITORING: ";
                std::clog << "Estou enviando status request para o client de ip: " << ip << std::endl;
                #endif
                
                packetSenderThreadStatus.emplace_back(&MonitoringSS::sendSleepStatusPackets, this, ip);
            }
            
            for (auto& packetSenderThreadStatus : packetSenderThreadStatus) {
                packetSenderThreadStatus.join();
            }

            #ifdef DEBUG
            std::clog << "MONITORING: ";
            std::clog << "Encerrei minhas threads " << std::endl;
            std::clog << "Vou dormir por 2 segundos" << std::endl;
            #endif

            std::this_thread::sleep_for(std::chrono::seconds(2));

            packetSenderThreadStatus.clear();
        }
        else { // Server - responde os pacotes de sleep status requests

            #ifdef DEBUG
            std::clog << "MONITORING: ";
            std::clog << "Estou esperando um packet em " << MONITORING_PORT << std::endl;
            #endif

            // Fica esperando por pacotes do manager
            std::string managerIp;
            u_int16_t managerPort;

            packet sendPacketStatusRequest, recvPacketStatusRequest;

            if(!monitoringSocket.receivePacket(recvPacketStatusRequest, managerPort, managerIp)){
                #ifdef DEBUG
                std::clog << "Timeout..." << std::endl;
                #endif                
                continue;
            }
            
            #ifdef DEBUG
            std::clog << "MONITORING: ";
            std::clog << "Recebi um pacote de " << managerIp << "!" << std::endl;
            #endif

            // Checa se o tipo do pacote está correto e responde
            if(recvPacketStatusRequest.type == SLEEP_STATUS_REQUEST) {
                #ifdef DEBUG
                std::clog << "MONITORING: ";
                std::clog << "Estou respondendo o manager " << managerIp << " sobre meu status" << std::endl;
                #endif

                // Retorna para o servidor pacote confirmando que ele recebeu o pacote de status
                sendPacketStatusRequest.type = SLEEP_STATUS_REQUEST | ACKNOWLEDGE;
                monitoringSocket.sendPacket(sendPacketStatusRequest, DIRECT_TO_IP, managerPort, &managerIp);
            }
        }
    }
    
    


};

void MonitoringSS::sendSleepStatusPackets(std::string ipstr){
    packet sendPacketSleepStatus, recvPacketSleepStatus;

    std::string participantIp;
    uint16_t participantPort;

    bool awake = false;
    sendPacketSleepStatus.type = SLEEP_STATUS_REQUEST;

    // Manda o pacote de sleepStatus enquanto não recebe confirmação e não há timeOut
    monitoringSocket.sendPacket(sendPacketSleepStatus, DIRECT_TO_IP, MONITORING_PORT, &ipstr);
    
    if (monitoringSocket.receivePacket(recvPacketSleepStatus, participantPort, participantIp)){
        if(recvPacketSleepStatus.type == (SLEEP_STATUS_REQUEST | ACKNOWLEDGE)) {
            #ifdef DEBUG
            std::clog << "MONITORING: ";
            std::clog << "Recebi um pacote do cliente com o status awake" << std::endl;
            #endif
            awake = true;
        } 
    }
   
    // Caso tenha timeOut atualiza o status para sleep do contrário atualiza para awake
    std::string message;

    tableManager->updateClient(awake, ipstr);
};