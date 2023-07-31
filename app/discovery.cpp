#include "discovery.h"

void DiscoverySS::start(){
    discoverySocket.openSocket();
    discoverySocket.setSocketTimeoutMS(100);

    if(isManager()){ // Server
        discoverySocket.bindSocket();
    }
    else{ // Client
        discoverySocket.setSocketBroadcastToTrue(); 
    }
    WOLSubsystem::start();
}

void DiscoverySS::stop(){
    
    if(isManager()){
        WOLSubsystem::stop();
    }
    else{
        // Espera a saída do sistema
        if(runThread != NULL){
            runThread->join();
            delete runThread;
        }
    }

    discoverySocket.closeSocket();
}


void DiscoverySS::run(){
    while(isRunning()){
        packet recvPacket, sendPacket;

        if(isManager()){ // Server - os pacotes de sleep service discovery e sleep service exit

            // Fica esperando por pacotes de algum cliente
            sockaddr_in clientAddrIn;

            /*
            if(!discoverySocket.receivePacketFromClients(&recvPacket, clientAddrIn)){
                // Timeout
                continue;
            }
            */

            try{
                if(!discoverySocket.receivePacketFromClients(&recvPacket, clientAddrIn)){
                    continue;
                }
            } catch(const std::runtime_error& e) {
                #ifdef DEBUG
                std::clog << "DISCOVERY: ";
                std::clog << "Exceção capturada na thread de receber pacote " << e.what() << std::endl;
                #endif
            }

            #ifdef DEBUG
            char buffer[INET_ADDRSTRLEN];
            inet_ntop( AF_INET, &clientAddrIn.sin_addr, buffer, sizeof( buffer ));
            std::clog << "DISCOVERY: ";
            std::clog << "Recebi um pacote de " << buffer << "!" << std::endl;
            #endif

            // Checa o tipo de pacote: Adicionar ao sistema ou retirar do sistema
            if(recvPacket.type == (SLEEP_SERVICE_DISCOVERY | SLEEP_SERVICE_DISCOVERY_FIND)){
                std::string macAndHostnameClient;
                macAndHostnameClient = recvPacket._payload;

                #ifdef DEBUG
                std::clog << "DISCOVERY: ";
                std::clog << "Estou respondendo o cliente " << buffer << " que quer entrar" << std::endl;
                #endif
                
                // Retorna para o cliente pacote informando que este é o manager, com suas informações de hostname e MAC
                sendPacket.type = SLEEP_SERVICE_DISCOVERY | SLEEP_SERVICE_DISCOVERY_FIND | ACKNOWLEDGE;
                std::string packetPayload = getHostname() + "&" + getMACAddress();
                strcpy(sendPacket._payload, packetPayload.c_str());

                discoverySocket.sendPacketToClient(&sendPacket, clientAddrIn);

                // Adiciona cliente à tabela

                // Envia mensagem para o gerenciamento para adicionar o cliente à tabela
                std::string message;
                char ipStr[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &clientAddrIn.sin_addr, ipStr, INET_ADDRSTRLEN);
                
                // Info
                IpInfo ipInfo;
                // Separa hostname e mac da string contida no packet
                std::string hostname, mac;
                hostname = macAndHostnameClient.substr(0, macAndHostnameClient.find('&'));
                mac = macAndHostnameClient.substr(macAndHostnameClient.find('&')+1);
                ipInfo.hostname = hostname;
                ipInfo.mac = mac;
                ipInfo.awake = true; // Por default, awake

                tableManager->insertClient(ipStr, ipInfo);

            }

            else if(recvPacket.type == (SLEEP_SERVICE_DISCOVERY | SLEEP_SERVICE_DISCOVERY_EXIT)){ 
                #ifdef DEBUG
                std::clog << "DISCOVERY: ";
                std::clog << "Estou respondendo o cliente " << buffer << " que quer sair" << std::endl;
                #endif
                
                // Retorna para o cliente pacote confirmando que ele recebeu o pacote de descoberta
                sendPacket.type = SLEEP_SERVICE_DISCOVERY | SLEEP_SERVICE_DISCOVERY_EXIT | ACKNOWLEDGE;
                discoverySocket.sendPacketToClient(&sendPacket, clientAddrIn);

                // Remove da tabela
                char ipStr[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &clientAddrIn.sin_addr, ipStr, INET_ADDRSTRLEN);

                tableManager->removeClient(ipStr);


            }
    
        }
        else{ // Client - envia os pacotes de sleep service discovery e sleep service exit
            sockaddr_in serverAddrIn;
            
            if(g_exiting){ // Verifica se é necessário mandar packet informando saída do sistema

                // O cliente já sabe qual o endereço do servidor
                discoverySocket.setSocketBroadcastToFalse(); 

                // Envia pacote
                sendSleepExitPackets(serverAddrIn);
                // Espera resposta do servidor

                //*******************************************************
                /*
                if(!discoverySocket.receivePacketFromServer(&recvPacket)){
                    // Timeout
                    continue;
                }
                */
                try{
                    if(!discoverySocket.receivePacketFromServer(&recvPacket)){
                    // Timeout
                    continue;
                    }
                } catch(const std::runtime_error& e) {
                    #ifdef DEBUG
                    std::clog << "DISCOVERY: ";
                    std::clog << "Exceção capturada na thread de confirmação da saída! " << e.what() << std::endl;
                    #endif
                }

                // Checa se é resposta do manager
                if(recvPacket.type == (SLEEP_SERVICE_DISCOVERY | SLEEP_SERVICE_DISCOVERY_EXIT | ACKNOWLEDGE)){
                    #ifdef DEBUG
                    std::clog << "DISCOVERY: ";
                    std::clog << "Recebi um pacote do manager de confirmação da saída!" << std::endl;
                    #endif
                }

                // Finalmente, encerra o subsistema
                setRunning(false);

            }

            if (!foundManager){ // Busca o manager

                // Envia pacote
                sendSleepDiscoverPackets();
                // Espera resposta do servidor
                //***************************************************************
                /*
                if(!discoverySocket.receivePacketFromServer(&recvPacket)){
                    // Timeout
                    continue;
                }
                */

                try{
                    if(!discoverySocket.receivePacketFromServer(&recvPacket)){
                    // Timeout
                    continue;
                    }
                } catch(const std::runtime_error& e) {
                    #ifdef DEBUG
                    std::clog << "DISCOVERY: ";
                    std::clog << "Exceção capturada na thread de confirmação que eu entrei! " << e.what() << std::endl;
                    #endif
                }
                
                serverAddrIn.sin_addr = discoverySocket.getServerBinaryNetworkAddress();

                // Checa se é resposta do manager
                if(recvPacket.type == (SLEEP_SERVICE_DISCOVERY | SLEEP_SERVICE_DISCOVERY_FIND | ACKNOWLEDGE)){
                    #ifdef DEBUG
                    std::clog << "DISCOVERY: ";
                    std::clog << "Recebi um pacote do manager de confirmação que eu entrei!" << std::endl;
                    #endif
                }

                // TO-DO: Duplicado. Ver como resolver isso.  
                // Adiciona o manager à tabela
                std::string macAndHostnameManager;
                macAndHostnameManager = recvPacket._payload;

                std::string message;
                IpInfo ipInfo;

                char ipStr[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &serverAddrIn.sin_addr, ipStr, INET_ADDRSTRLEN);

                // Separa hostname e mac da string contida no packet
                std::string hostname, mac;
                hostname = macAndHostnameManager.substr(0, macAndHostnameManager.find('&'));
                mac = macAndHostnameManager.substr(macAndHostnameManager.find('&')+1);
                ipInfo.hostname = hostname;
                ipInfo.mac = mac;
                ipInfo.awake = true; // Por default, awake

                tableManager->insertClient(ipStr, ipInfo);
                
                foundManager = true;
            }
        }
    }
};

void DiscoverySS::sendSleepDiscoverPackets(){
    packet sendPacket;

    sendPacket.type = SLEEP_SERVICE_DISCOVERY | SLEEP_SERVICE_DISCOVERY_FIND;
    std::string packetPayload = getHostname() + "&" + getMACAddress();
    strcpy(sendPacket._payload, packetPayload.c_str());

    #ifdef DEBUG
    std::clog << "DISCOVERY: ";
    std::clog << "Enviando packet de procura..." << std::endl;
    #endif

    discoverySocket.sendPacketToServer(&sendPacket, BROADCAST, NULL);
}

void DiscoverySS::sendSleepExitPackets(struct sockaddr_in serverAddrIn){
    packet sendPacket;
    sendPacket.type = SLEEP_SERVICE_DISCOVERY | SLEEP_SERVICE_DISCOVERY_EXIT;

    #ifdef DEBUG
    std::clog << "DISCOVERY: ";
    std::clog << "Enviando packet de saída..."<< std::endl;
    #endif
    discoverySocket.sendPacketToServer(&sendPacket, DIRECT_TO_SERVER, &serverAddrIn);

}

std::string DiscoverySS::getHostname(){
    std::string hostname; // String vazia significa que hostname não foi definido
    std::ifstream hostname_file;
    hostname_file.open("/etc/hostname");

    if(hostname_file.is_open()){
        getline(hostname_file, hostname); 
    }
    return hostname;
}

std::string DiscoverySS::getMACAddress(){
    int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    std::string macaddr_str;
    ifreq iface;

    if(sockfd >= 0){
        strcpy(iface.ifr_name, "eth0");

        if (ioctl(sockfd, SIOCGIFHWADDR, &iface) == 0) {
            char iface_str [18];
            sprintf(iface_str, "%02x:%02x:%02x:%02x:%02x:%02x",
                    (unsigned char) iface.ifr_addr.sa_data[0],
                    (unsigned char) iface.ifr_addr.sa_data[1],
                    (unsigned char) iface.ifr_addr.sa_data[2],
                    (unsigned char) iface.ifr_addr.sa_data[3],
                    (unsigned char) iface.ifr_addr.sa_data[4],
                    (unsigned char) iface.ifr_addr.sa_data[5]);
            macaddr_str = iface_str;
        }
    }
    return macaddr_str;
}
