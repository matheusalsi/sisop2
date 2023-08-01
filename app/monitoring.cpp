#include "monitoring.h"

void MonitoringSS::start(){
    monitoringSocket.oldOpenSocket();
     
    monitoringSocket.setSocketTimeoutMS(100);

    if(!isManager()){ // Participante (Servidor)
        monitoringSocket.oldBindSocket();
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
                char ipStr[INET_ADDRSTRLEN];
                
                strcpy(ipStr, ip.c_str());
                #ifdef DEBUG
                std::clog << "MONITORING: ";
                std::clog << "Estou enviando status request para o client de ip: " << ipStr << std::endl;
                #endif

                
                packetSenderThreadStatus.emplace_back(&MonitoringSS::sendSleepStatusPackets, this, ipStr);
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

            if(!monitoringSocket.oldreceivePacketFromClients(&recvPacketStatusRequest, managerAddrIn)){
                #ifdef DEBUG
                std::clog << "Timeout..." << std::endl;
                #endif                
                continue;
            }
            
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
                monitoringSocket.oldsendPacketToClient(&sendPacketStatusRequest, managerAddrIn);
            }
        }
    }
    
    


};

void MonitoringSS::sendSleepStatusPackets(char ipStr[INET_ADDRSTRLEN]){
    packet sendPacketSleepStatus, recvPacketSleepStatus;
    sockaddr_in managerAddrin;
    bool asleep = true;
    sendPacketSleepStatus.type = SLEEP_STATUS_REQUEST;

    sockaddr_in clientAddrIn;
    inet_pton(AF_INET, ipStr, &(clientAddrIn.sin_addr));

    // Manda o pacote de sleepStatus enquanto não recebe confirmação e não há timeOut
    monitoringSocket.oldsendPacketToServer(&sendPacketSleepStatus, DIRECT_TO_IP, &clientAddrIn);

    try {
        if (monitoringSocket.oldreceivePacketFromClients(&recvPacketSleepStatus, managerAddrin)){
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

    inet_ntop(AF_INET, &clientAddrIn.sin_addr, ipStr, INET_ADDRSTRLEN);
    tableManager->updateClient(!asleep, ipStr);

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