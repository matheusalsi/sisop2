#include "monitoring.h"

void MonitoringSS::start(){
    monitoringSocket.openSocket();
    monitoringSocket.setSocketTimeoutMS(100); 

    if(!isManager()){ // Cliente
        monitoringSocket.bindSocket();
    }
    WOLSubsystem::start();
};

void MonitoringSS::stop(){
    WOLSubsystem::stop();
    monitoringSocket.closeSocket();
};

void MonitoringSS::run(){
    packet recvPacket, sendPacket;

    while (isRunning()) {
        if(isManager()){ // Cliente - envia os pacotes de sleep status requests
            
            while (isRunning() && !mailBox.isEmpty("D_OUT -> MO_IN")){ // Pega todas as mensagens da caixa do discovery e adiciona ao conjunto de IPs
                std::string messageClientIp;
                mailBox.readMessage("D_OUT -> MO_IN", messageClientIp);
                
                if (ipList.find(messageClientIp) == ipList.end()) 
                    ipList.insert(messageClientIp);
                else // Caso o discovery mande um IP que já existe no conjunto, é um aviso de que esse cliente saiu do sistema
                    ipList.erase(messageClientIp);
            }
            
            if (ipList.size() == 0) {
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

            
            for(auto ip: ipList) {
                char ipStr[INET_ADDRSTRLEN];
                strcpy(ipStr, ip.c_str());
                #ifdef DEBUG
                std::clog << "MONITORING: ";
                std::clog << "Estou enviando status request para o client de ip: " << ipStr << std::endl;
                #endif

                sockaddr_in clientAddrIn;
                inet_pton(AF_INET, ipStr, &(clientAddrIn.sin_addr));
                packetSenderThreadStatus.emplace_back(&MonitoringSS::sendSleepStatusPackets, this, clientAddrIn);
            }
            
            for (auto& packetSenderThreadStatus : packetSenderThreadStatus) {
                packetSenderThreadStatus.join();
            }

            #ifdef DEBUG
            std::clog << "MONITORING: ";
            std::clog << "Encerrei minhas threads " << std::endl;
            #endif

            packetSenderThreadStatus.clear();
        }
        else { // Server - responde os pacotes de sleep status requests

            #ifdef DEBUG
            std::clog << "MONITORING: ";
            std::clog << "Estou esperando um packet em " << MONITORING_PORT << std::endl;
            #endif

            // Fica esperando por pacotes do manager
            sockaddr_in managerAddrIn;
            if(!monitoringSocket.receivePacketFromClients(&recvPacket, managerAddrIn)){
                // Timeout
                continue;
            }

            #ifdef DEBUG
            char ipStr[INET_ADDRSTRLEN];
            inet_ntop( AF_INET, &managerAddrIn.sin_addr, ipStr, sizeof( ipStr ));
            std::clog << "MONITORING: ";
            std::clog << "Recebi um pacote de " << ipStr << "!" << std::endl;
            #endif

            // Checa se o tipo do pacote está correto e responde
            if(recvPacket.type == SLEEP_STATUS_REQUEST) {
                #ifdef DEBUG
                std::clog << "MONITORING: ";
                std::clog << "Estou respondendo o manager " << ipStr << " sobre meu status" << std::endl;
                #endif

                // Retorna para o servidor pacote confirmando que ele recebeu o pacote de status
                sendPacket.type = SLEEP_STATUS_REQUEST | ACKNOWLEDGE;
                monitoringSocket.sendPacketToClient(&sendPacket, managerAddrIn);
            }
        }
    }
    
    


};

void MonitoringSS::sendSleepStatusPackets(struct sockaddr_in managerAddrIn){
    packet sendPacket, recvPacket;
    sockaddr_in clientAddrin;
    bool replied = false;
    bool timerExpired = false;
    sendPacket.type = SLEEP_STATUS_REQUEST;

    std::chrono::seconds timeout(1);
    auto startTime = std::chrono::steady_clock::now();

    // Manda o pacote de sleepStatus enquanto não recebe confirmação e não há timeOut
    while(!timerExpired && !replied && isRunning()){
        recvPacket.type = SLEEP_STATUS_REQUEST;
        monitoringSocket.sendPacketToServer(&sendPacket, DIRECT_TO_SERVER, &managerAddrIn);

        try {
            monitoringSocket.receivePacketFromClients(&recvPacket, clientAddrin);
        } catch(const std::runtime_error& e) {
            #ifdef DEBUG
            std::clog << "MONITORING: ";
            std::clog << "Exceção capturada na thread de sleep status packets: " << e.what() << std::endl;
            #endif
        }

        if(recvPacket.type == (SLEEP_STATUS_REQUEST | ACKNOWLEDGE)) {
            #ifdef DEBUG
            std::clog << "MONITORING: ";
            std::clog << "Recebi um pacote do cliente com o status awake" << std::endl;
            #endif
            replied = true;
        }
        auto currentTime = std::chrono::steady_clock::now();
        auto elapsedTime = std::chrono::duration_cast<std::chrono::seconds>(currentTime - startTime);

        if(elapsedTime >= timeout) {
            timerExpired = true;
        }
    }
    // Caso tenha timeOut atualiza o status para sleep do contrário atualiza para awake
    std::string message;
    char ipStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &managerAddrIn.sin_addr, ipStr, INET_ADDRSTRLEN);
    message.append("UPDATE_CLIENT_STATUS");
    message.append("&");
    message.append(ipStr);
    message.append("&");
    
    if (timerExpired && !replied)
        message.append("asleep");
    
    else if (replied)
        message.append("awake");
    
    mailBox.writeMessage("M_IN <- MO_OUT", message);
};

// // Le a mensagem com a lista de ips clientes da tabela de gerenciamento com formato 17.172.224.47&17.172.224.48&17.172.224.49
// void MonitoringSS::setIpList(std::vector<std::string>& ipList, std::string messageClientsIps){
//     ipList.clear();
    
//     if (messageClientsIps.find('&') == std::string::npos) {
//         ipList.push_back(messageClientsIps);
//     }
//     else {
//         size_t start = 0;
//         size_t end = messageClientsIps.find('&');
//         while (end != std::string::npos) {
//             std::string ip = messageClientsIps.substr(start, end - start);
//             ipList.push_back(ip);
//             start = end + 1;
//             end = messageClientsIps.find('&',start);
//         }
//         std::string ip = messageClientsIps.substr(start);
//         ipList.push_back(ip);
//     }
//     this->ipList = ipList;
// };

// std::vector<std::string> MonitoringSS::getIplist(){
//     return ipList;
// };