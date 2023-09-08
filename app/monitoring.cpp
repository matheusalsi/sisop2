#include "monitoring.h"

void MonitoringSS::start(){
    monitoringSocket.openSocket();
    monitoringSocket.setSocketTimeoutMS(MONITORING_TIMEOUT_MS);
    monitoringSocket.bindSocket();
    WOLSubsystem::start();
};

void MonitoringSS::stop(){
    WOLSubsystem::stop();
    monitoringSocket.closeSocket();
};

void MonitoringSS::runAsManager(){
    // Obtém IPs da tabela
    const auto ipList = tableManager->getKnownIps();
    
    // Se houver apenas um IP, é ele mesmo
    if (ipList->size() <= 1) {
        #ifdef DEBUG
        std::clog << "MONITORING: ";
        std::clog << "Não há clientes para monitorar" << std::endl;
        #endif
        std::this_thread::sleep_for(std::chrono::seconds(1));
        return;
    }

    // Reseta mapa de acordados
    for(auto ip : *ipList){
        awakeStatus[ip] = false;
    }

    // Threads que fazem envio e resposta de pacotes
    std::vector<std::thread> packetSenderThreadStatus;
    
    for(auto ip: *ipList) {
        if(ip == g_myIP){
            // Obviamente o próprio manager está acordado
            awakeStatus[ip] = true;
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
        // Se descobrimos que o cliente recém acordou, reenviamos a tabela
        if(!tableManager->isClientAwake(pair.first) && pair.second){
            logger.log(std::string("MONITORING: Reenviando tabela a ") + pair.first);
            tableManager->sendTableToIP(pair.first);
        }

        tableManager->updateClient(pair.second, pair.first);
    }

    // Dorme por 2 segundos
    std::this_thread::sleep_for(std::chrono::seconds(2));

    packetSenderThreadStatus.clear();
}

void MonitoringSS::runAsClient(){
    // Eleição via timeout de monitoramento só pode ocorrer se já foi encontrado um manager
    if(!g_foundManager){
        return;
    }

    // Fica esperando por pacotes do manager, mas pode também
    // receber uma resposta do manager antigo
    std::string answeringIP;
    u_int16_t managerPort;
    packet sendPacketStatusRequest, recvPacketStatusRequest;

    int nTimeouts = 0;
    while(isRunning()){
        if(nTimeouts == MAX_MANAGER_MONITORING_TIMEOUTS){
            // Entra em modo eleição
            g_electionHappening = true;
            return;
        }
        
        if(!monitoringSocket.receivePacket(recvPacketStatusRequest, managerPort, answeringIP)){
            nTimeouts++;
            continue;
        }
        else{
            break;
        }
    }    

    // Checa se o tipo do pacote está correto e responde
    if(recvPacketStatusRequest.type == SLEEP_STATUS_REQUEST) {
        // Checa se o IP que mandou o pacote é o gerenciador
        std::string managerIP = tableManager->getManagerIP();
        if(answeringIP == managerIP){
            sendPacketStatusRequest.type = SLEEP_STATUS_REQUEST | ACKNOWLEDGE;
            monitoringSocket.sendPacket(sendPacketStatusRequest, DIRECT_TO_IP, managerPort, &managerIP);
        }
        // Se não é, respondemos com o IP do gerenciador
        else{
            // Retorna para o servidor pacote confirmando que ele recebeu o pacote de status
            sendPacketStatusRequest.type = SLEEP_STATUS_REQUEST | SLEEP_STATUS_REQUEST_CORRECTION;
            strcpy(sendPacketStatusRequest._payload, managerIP.c_str());
            monitoringSocket.sendPacket(sendPacketStatusRequest, DIRECT_TO_IP, managerPort, &managerIP);
        }
    }
}

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