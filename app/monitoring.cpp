#include "monitoring.h"

void MonitoringSS::start(){
    monitoringSocket.openSocket();

    if(isManager()){ // Server
        monitoringSocket.bindSocket();
    }
    else{ // Client
        monitoringSocket.setSocketBroadcastToFalse(); 
    }
    WOLSubsystem::start();
};

void MonitoringSS::stop(){
    monitoringSocket.closeSocket();
};

void MonitoringSS::run(){
    packet recvPacket, sendPacket;
    while (isRunning()) {
        if(isManager()){ // Server - envia os pacotes de sleep status requests
            #ifdef DEBUG
            std::cout << "Estou enviando um packet em " << MONITORING_PORT << std::endl;
            #endif

            while (mailBox.isEmpty("MO_IN")){ // Espera chegar alguma mensagem na caixa de mensagens
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }  
                
            std::string messageClientsIps;
            mailBox.readMessage("MO_IN", messageClientsIps);
            setIpList(ipList , messageClientsIps);

            std::vector<sockaddr_in> clientAddrIn;
            std::vector<std::thread> packetSenderThreadStatus;
            
            for(unsigned int i = 0; i < ipList.size(); i++) {
                char ipStr[INET_ADDRSTRLEN];
                strcpy(ipStr, this->ipList[i].c_str());
                #ifdef DEBUG
                std::cout << "Estou enviando status request para o client de ip: " << ipStr << std::endl;
                #endif
                inet_pton(AF_INET, ipStr, &(clientAddrIn[i].sin_addr));
                packetSenderThreadStatus.emplace_back(&MonitoringSS::sendSleepStatusPackets, this, clientAddrIn[i]);
            }
            
            for (auto& packetSenderThreadStatus : packetSenderThreadStatus) {
                packetSenderThreadStatus.join();
            }

            clientAddrIn.clear();
            packetSenderThreadStatus.clear();
        }
        else { // Client - responde os pacotes de sleep status requests

            #ifdef DEBUG
            std::cout << "Estou esperando um packet em " << MONITORING_PORT << std::endl;
            #endif

            // Fica esperando por pacotes do servidor
            sockaddr_in serverAddrIn;
            serverAddrIn = monitoringSocket.receivePacketFromServer(&recvPacket);

            #ifdef DEBUG
            char ipStr[INET_ADDRSTRLEN];
            inet_ntop( AF_INET, &serverAddrIn.sin_addr, ipStr, sizeof( ipStr ));
            std::cout << "Recebi um pacote de " << ipStr << "!" << std::endl;
            #endif

            // Checa se o tipo do pacote está correto e responde
            if(recvPacket.type == SLEEP_STATUS_REQUEST) {
                #ifdef DEBUG
                std::cout << "Estou respondendo o servidor " << ipStr << " sobre meu status" << std::endl;
                #endif

                // Retorna para o servidor pacote confirmando que ele recebeu o pacote de status
                sendPacket.type = SLEEP_STATUS_REQUEST | ACKNOWLEDGE;
                monitoringSocket.sendPacketToServer(&sendPacket, DIRECT_TO_SERVER, &serverAddrIn);
            }
            /*
            if(recvPacket.type == WAKE_UP){
                //todo - acordar máquina
            }
            */
        }
    }
    
    


};

void MonitoringSS::sendSleepStatusPackets(struct sockaddr_in clientAddrIn){
    packet sendPacket, recvPacket;
    bool replied = false;
    bool timerExpired = false;
    sendPacket.type = SLEEP_STATUS_REQUEST;

    std::chrono::seconds timeout(1);
    auto startTime = std::chrono::steady_clock::now();

    // Manda o pacote de sleepStatus enquanto não recebe confirmação e não há timeOut
    while(!timerExpired && !replied && isRunning()){
        auto currentTime = std::chrono::steady_clock::now();
        auto elapsedTime = std::chrono::duration_cast<std::chrono::seconds>(currentTime - startTime);

        if(elapsedTime >= timeout) {
            timerExpired = true;
        }

        #ifdef DEBUG
        std::cout << "Enviando packet de saída..."<< std::endl;
        #endif
        monitoringSocket.sendPacketToClient(&sendPacket, clientAddrIn);

        monitoringSocket.receivePacketFromClients(&recvPacket);

        if(recvPacket.type == (SLEEP_STATUS_REQUEST | ACKNOWLEDGE)) {
            std::cout << "Recebi um pacote do cliente com o status awake" << std::endl;
            replied = true;
        }
    }
    // Caso tenha timeOut atualiza o status para sleep do contrário atualiza para awake
    if (timerExpired && !replied){
        std::string message;
        char ipStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddrIn.sin_addr, ipStr, INET_ADDRSTRLEN);
        message.append("UPDATE_CLIENT_STATUS");
        message.append("&");
        message.append(ipStr);
        message.append("&");
        message.append(std::to_string(clientAddrIn.sin_port));
        message.append("&");
        message.append("awake");
        mailBox.writeMessage("M_IN", message);
    }
    else {
        std::string message;
        char ipStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddrIn.sin_addr, ipStr, INET_ADDRSTRLEN);
        message.append("UPDATE_CLIENT_STATUS");
        message.append("&");
        message.append(ipStr);
        message.append("&");
        message.append(std::to_string(clientAddrIn.sin_port));
        message.append("&");
        message.append("asleep");
        mailBox.writeMessage("M_IN", message);
    }
};

// Le a mensagem com a lista de ips clientes da tabela de gerenciamento com formato 17.172.224.47&17.172.224.48&17.172.224.49
void MonitoringSS::setIpList(std::vector<std::string>& ipList, std::string messageClientsIps){
    ipList.clear();
    
    if (messageClientsIps.find('&') == std::string::npos) {
        ipList.push_back(messageClientsIps);
    }
    else {
        size_t start = 0;
        size_t end = messageClientsIps.find('&');
        while (end != std::string::npos) {
            std::string ip = messageClientsIps.substr(start, end - start);
            ipList.push_back(ip);
            start = end + 1;
            end = messageClientsIps.find('&',start);
        }
        std::string ip = messageClientsIps.substr(start);
        ipList.push_back(ip);
    }
    this->ipList = ipList;
};

std::vector<std::string> MonitoringSS::getIplist(){
    return ipList;
};