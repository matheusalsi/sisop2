#include "monitoring.h"

void MonitoringSS::start(){
    monitoringSocket.openSocket();
     
    monitoringSocket.setSocketTimeoutMS(MONITORING_TIMEOUT_MS);

    if(!g_isManager){ // Participante (Servidor)
        monitoringSocket.bindSocket();
    }
    WOLSubsystem::start();
};

void MonitoringSS::stop(){
    WOLSubsystem::stop();
    monitoringSocket.closeSocket();
};

void MonitoringSS::run(){
    // Número de timeouts de espera por pacotes do manager
    static int nManagerTimeouts = 0;

    while (isRunning()) {

        // Lida com eleições
        if(g_electionHappening){

            continue;
        }




        if(g_isManager){ // Cliente - envia os pacotes de sleep status requests
            
            // Obtém IPs da tabela
            const auto ipList = tableManager->getKnownIps();
            
            // Se houver apenas um IP, é ele mesmo
            if (ipList->size() <= 1) {
                #ifdef DEBUG
                std::clog << "MONITORING: ";
                std::clog << "Não há clientes para monitorar" << std::endl;
                #endif
                std::this_thread::sleep_for(std::chrono::seconds(1));
                continue;
            }

            // Reseta mapa de acordados
            for(auto pair : awakeStatus){
                awakeStatus[pair.first] = false;
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
                if(ip == g_myIP){
                    continue;
                }
                
                packetSenderThreadStatus.emplace_back(&MonitoringSS::sendSleepStatusPackets, this, ip);
            }
            
            // Junta threads
            for (auto& packetSenderThreadStatus : packetSenderThreadStatus) {
                packetSenderThreadStatus.join();
            }

            // Atualiza tabela de acordo com respostas
            for(auto pair : awakeStatus){
                tableManager->updateClient(pair.second, pair.first);
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

            // Eleição via timeout de monitoramento só pode ocorrer se já foi encontrado um manager
            if(!g_foundManager){
                continue;
            }


            // Fica esperando por pacotes do manager, mas pode também
            // receber uma resposta do manager antigo
            std::string answeringIP;
            u_int16_t managerPort;

            packet sendPacketStatusRequest, recvPacketStatusRequest;

            // Excesso de timeouts leva a nova eleição
            if(nManagerTimeouts > MAX_MANAGER_MONITORING_TIMEOUTS){
                g_electionHappening = true;
                continue;
            }
            
            if(!monitoringSocket.receivePacket(recvPacketStatusRequest, managerPort, answeringIP)){
                #ifdef DEBUG
                std::clog << "Timeout..." << std::endl;
                #endif
                nManagerTimeouts++;                
                continue;
            }
            
            // Reseta nº de timeouts
            nManagerTimeouts = 0;
            
            #ifdef DEBUG
            std::clog << "MONITORING: ";
            std::clog << "Recebi um pacote de " << answeringIP << "!" << std::endl;
            #endif

            // Checa se o tipo do pacote está correto e responde
            if(recvPacketStatusRequest.type == SLEEP_STATUS_REQUEST) {

                // Checa se o IP que mandou o pacote é o gerenciador
                std::string managerIP = tableManager->getManagerIP();
                if(answeringIP == managerIP){
                    #ifdef DEBUG
                    std::clog << "MONITORING: ";
                    std::clog << "Estou respondendo o manager " << managerIP << " sobre meu status" << std::endl;
                    #endif
                    sendPacketStatusRequest.type = SLEEP_STATUS_REQUEST | ACKNOWLEDGE;
                    monitoringSocket.sendPacket(sendPacketStatusRequest, DIRECT_TO_IP, managerPort, &managerIP);
                }
                // Se não é, respondemos com o IP do gerenciador
                else{
                    // Retorna para o servidor pacote confirmando que ele recebeu o pacote de status
                    // TO-DO: Mandar também a tabela atual
                    #ifdef DEBUG
                    std::clog << "MONITORING: ";
                    std::clog << "Um possivel manager antigo (" << managerIP << ") esta me monitorando; Irei atualiza-lo" << std::endl;
                    #endif
                    sendPacketStatusRequest.type = SLEEP_STATUS_REQUEST | SLEEP_STATUS_REQUEST_CORRECTION;
                    strcpy(sendPacketStatusRequest._payload, managerIP.c_str());
                    monitoringSocket.sendPacket(sendPacketStatusRequest, DIRECT_TO_IP, managerPort, &managerIP);
                }

            }
        }
    }
    
    


};

void MonitoringSS::sendSleepStatusPackets(std::string ipstr){
    packet sendPacketSleepStatus, recvPacketSleepStatus;

    std::string participantIp;
    uint16_t participantPort;

    sendPacketSleepStatus.type = SLEEP_STATUS_REQUEST;

    // Manda um pacote sleep status request para esse IP
    monitoringSocket.sendPacket(sendPacketSleepStatus, DIRECT_TO_IP, MONITORING_PORT, &ipstr);
    
    // Pega uma resposta de algum participante, não necessariamente de quem mandamos
    if (monitoringSocket.receivePacket(recvPacketSleepStatus, participantPort, participantIp)){
        if(recvPacketSleepStatus.type == (SLEEP_STATUS_REQUEST | ACKNOWLEDGE)) {
            // Atualizamos esse participante como acordado
            awakeStatus[participantIp] = true;
        } 
    }

};