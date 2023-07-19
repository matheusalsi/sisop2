#include "monitoring.h"

void MonitoringSS::start(){
    monitoringSocket.openSocket();
     
    if(!isManager()){ // Participante (Servidor)
        monitoringSocket.bindSocket();
    }
    else{ // Manager (Cliente)
        monitoringSocket.setSocketTimeoutMS(100);
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
            
            // Sincronização com gerenciamento
            while (isRunning() && !mailBox.isEmpty("M_OUT -> MO_IN")){
                std::string messageIpUpdates;
                mailBox.readMessage("M_OUT -> MO_IN", messageIpUpdates);
                #ifdef DEBUG
                std::clog << "MONITORING: ";
                std::clog << "Mensagem de GERENCIAMENTO: " << messageIpUpdates << std::endl;
                #endif
                

                char type = messageIpUpdates[0]; // Primeiro caractere determina se IP está entrando ou saíndo
                std::string ip = messageIpUpdates.substr(1);

                // Atualiza ips a monitorar
                if(type == '+'){
                    ipList.insert(ip);
                }
                else{
                    ipList.erase(ip);
                }

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
            sockaddr_in managerAddrIn;
            packet sendPacketStatusRequest, recvPacketStatusRequest;

            monitoringSocket.receivePacketFromClients(&recvPacketStatusRequest, managerAddrIn);
            
            #ifdef DEBUG
            char ipStr[INET_ADDRSTRLEN];
            inet_ntop( AF_INET, &managerAddrIn.sin_addr, ipStr, sizeof( ipStr ));
            std::clog << "MONITORING: ";
            std::clog << "Recebi um pacote de " << ipStr << "!" << std::endl;
            #endif

            // Checa se o tipo do pacote está correto e responde
            if(recvPacketStatusRequest.type == SLEEP_STATUS_REQUEST) {
                #ifdef DEBUG
                std::clog << "MONITORING: ";
                std::clog << "Estou respondendo o manager " << ipStr << " sobre meu status" << std::endl;
                #endif

                // Retorna para o servidor pacote confirmando que ele recebeu o pacote de status
                sendPacketStatusRequest.type = SLEEP_STATUS_REQUEST | ACKNOWLEDGE;
                monitoringSocket.sendPacketToClient(&sendPacketStatusRequest, managerAddrIn);
            }
        }
    }
    
    


};

void MonitoringSS::sendSleepStatusPackets(struct sockaddr_in managerAddrIn){
    packet sendPacketSleepStatus, recvPacketSleepStatus;
    sockaddr_in clientAddrin;
    bool asleep = true;
    sendPacketSleepStatus.type = SLEEP_STATUS_REQUEST;

    // Manda o pacote de sleepStatus enquanto não recebe confirmação e não há timeOut
    monitoringSocket.sendPacketToServer(&sendPacketSleepStatus, DIRECT_TO_SERVER, &managerAddrIn);

    try {
    if (monitoringSocket.receivePacketFromClients(&recvPacketSleepStatus, clientAddrin)){

        if(recvPacketSleepStatus.type == (SLEEP_STATUS_REQUEST | ACKNOWLEDGE)) {
            #ifdef DEBUG
            std::clog << "MONITORING: ";
            std::clog << "Recebi um pacote do cliente com o status awake" << std::endl;
            #endif
            asleep = false;
        } 
    }
    } 
    catch(const std::runtime_error& e) {
        #ifdef DEBUG
        std::clog << "MONITORING: ";
        std::clog << "Exceção capturada na thread de sleep status packets: " << e.what() << std::endl;
        #endif
    }


    // Caso tenha timeOut atualiza o status para sleep do contrário atualiza para awake
    std::string message;
    char ipStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &managerAddrIn.sin_addr, ipStr, INET_ADDRSTRLEN);
    message.append("UPDATE_CLIENT_STATUS");
    message.append("&");
    message.append(ipStr);
    message.append("&");
    
    if (asleep)
        message.append("asleep");
    else
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