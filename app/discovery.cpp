#include "discovery.h"

void DiscoverySS::start(){
    discoverySocket.openSocket();

    if(isManager()){ // Server
        discoverySocket.bindSocket();
    }
    else{ // Client
        discoverySocket.setSocketBroadcastToTrue(); 
    }
    WOLSubsystem::start();
}

void DiscoverySS::stop(){
    discoverySocket.closeSocket();
}


void DiscoverySS::run(){
    while(isRunning()){
        packet recvPacket, sendPacket;

        if(isManager()){ // Server - os pacotes de sleep service discovery e sleep service exit

            #ifdef DEBUG
            std::clog << "DISCOVERY: ";
            std::clog << "Estou esperando um packet em " << DISCOVERY_PORT << std::endl;
            #endif

            // Fica esperando por pacotes de algum cliente
            sockaddr_in clientAddrIn;
            clientAddrIn = discoverySocket.receivePacketFromClients(&recvPacket);

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

                // Envia mensagem para o gerenciamento para adicionar o cliente à tabela
                std::string message;
                char ipStr[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &clientAddrIn.sin_addr, ipStr, INET_ADDRSTRLEN);
                message.append("ADD_CLIENT");
                message.append("&");
                message.append(ipStr);
                message.append("&");
                message.append(macAndHostnameClient);
                mailBox.writeMessage("M_IN <- D_OUT", message);

                // Envia mensagem para o monitoramento adicionando o ip do cliente a lista
                mailBox.writeMessage("MO_IN <- D_OUT", ipStr); 

            }

            else if(recvPacket.type == (SLEEP_SERVICE_DISCOVERY | SLEEP_SERVICE_DISCOVERY_EXIT)){ 
                #ifdef DEBUG
                std::clog << "DISCOVERY: ";
                std::clog << "Estou respondendo o cliente " << buffer << " que quer sair" << std::endl;
                #endif
                
                // Retorna para o cliente pacote confirmando que ele recebeu o pacote de descoberta
                sendPacket.type = SLEEP_SERVICE_DISCOVERY | SLEEP_SERVICE_DISCOVERY_EXIT | ACKNOWLEDGE;
                discoverySocket.sendPacketToClient(&sendPacket, clientAddrIn);

                // Envia mensagem para o gerenciamento para remover o cliente da tabela
                std::string messageExit;
                char ipStr[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &clientAddrIn.sin_addr, ipStr, INET_ADDRSTRLEN);
                messageExit.append("REMOVE_CLIENT");
                messageExit.append("&");
                messageExit.append(ipStr);
                mailBox.writeMessage("M_IN <- D_OUT", messageExit);

                // Envia mensagem para o monitoramento para remover o ip do cliente a lista
                mailBox.writeMessage("MO_IN <- D_OUT", ipStr); 
            }
    
        }
        else{ // Client - envia os pacotes de sleep service discovery e sleep service exit
            sockaddr_in serverAddrIn;
            if(foundManager && !hasLeft){ // Verifica se é necessário mandar packet informando saída do sistema

                while (mailBox.isEmpty("I_OUT -> D_IN")){ // Espera chegar alguma mensagem na caixa de mensagens
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
                std::string messageInterfaceLeave;
                mailBox.readMessage("I_OUT -> D_IN", messageInterfaceLeave);

                #ifdef DEBUG
                std::clog << "DISCOVERY: ";
                std::clog << "Mensagem de INTERFACE: " << messageInterfaceLeave << std::endl; 
                #endif

                // O cliente já sabe qual o endereço do servidor
                discoverySocket.setSocketBroadcastToFalse(); 

                // Envia pacote em outra thread informando que o cliente está saindo
                std::thread packetSenderThreadExit(&DiscoverySS::sendSleepExitPackets, this, serverAddrIn);

                // Espera resposta do servidor
                discoverySocket.receivePacketFromServer(&recvPacket);

                // Checa se é resposta do manager
                if(recvPacket.type == (SLEEP_SERVICE_DISCOVERY | SLEEP_SERVICE_DISCOVERY_EXIT | ACKNOWLEDGE)){
                    #ifdef DEBUG
                    std::clog << "DISCOVERY: ";
                    std::clog << "Recebi um pacote do manager de confirmação da saída!" << std::endl;
                    #endif
                }

                hasLeft = true; // Seta como true, acaba encerrando a thread "packet sender"
                packetSenderThreadExit.join(); // Espera a thread encerrar

                // Avisa para a interface que o cliente foi removido
                #ifdef DEBUG
                std::clog << "DISCOVERY: ";
                std::clog << "Estou avisando a minha interface que eu fui removido" << std::endl;
                #endif
                std::string messageTableRemoved;
                messageTableRemoved.append("I_WAS_REMOVED");
                mailBox.writeMessage("I_IN <- D_OUT", messageTableRemoved);
                // setRunning(false); // Discovery encerra a execução do participante
            }
            else if (!hasLeft){ // Busca o manager
                std::thread packetSenderThreadDiscover(&DiscoverySS::sendSleepDiscoverPackets, this);
                
                // Espera resposta do servidor
                serverAddrIn = discoverySocket.receivePacketFromServer(&recvPacket);

                // Checa se é resposta do manager
                if(recvPacket.type == (SLEEP_SERVICE_DISCOVERY | SLEEP_SERVICE_DISCOVERY_FIND | ACKNOWLEDGE)){
                    #ifdef DEBUG
                    std::clog << "DISCOVERY: ";
                    std::clog << "Recebi um pacote do manager de confirmação que eu entrei!" << std::endl;
                    #endif
                }

                // Adiciona o manager ao seu gerenciamento
                std::string macAndHostnameManager;
                macAndHostnameManager = recvPacket._payload;

                std::string message;
                char ipStr[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &serverAddrIn.sin_addr, ipStr, INET_ADDRSTRLEN);
                message.append("ADD_MANAGER");
                message.append("&");
                message.append(ipStr);
                message.append("&");
                message.append(macAndHostnameManager);
                mailBox.writeMessage("M_IN <- D_OUT", message);
                
                foundManager = true; // Seta como true, acaba encerrando a thread "packet sender"
                packetSenderThreadDiscover.join(); // Espera a thread encerrar
            }
        }
    }
};

void DiscoverySS::sendSleepDiscoverPackets(){
    packet sendPacket;
    sendPacket.type = SLEEP_SERVICE_DISCOVERY | SLEEP_SERVICE_DISCOVERY_FIND;

    std::string packetPayload = getHostname() + "&" + getMACAddress();

    strcpy(sendPacket._payload, packetPayload.c_str());

    // Manda o pacote de discovery enquanto não recebe confirmação
    while(!foundManager && isRunning()){
        #ifdef DEBUG
        std::clog << "DISCOVERY: ";
        std::clog << "Enviando packet de procura..." << std::endl;
        #endif
        discoverySocket.sendPacketToServer(&sendPacket, BROADCAST, NULL);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

void DiscoverySS::sendSleepExitPackets(struct sockaddr_in serverAddrIn){
    packet sendPacket;
    sendPacket.type = SLEEP_SERVICE_DISCOVERY | SLEEP_SERVICE_DISCOVERY_EXIT;
    // Manda o pacote de exit enquanto não recebe confirmação
    while(!hasLeft && isRunning()){
        #ifdef DEBUG
        std::clog << "DISCOVERY: ";
        std::clog << "Enviando packet de saída..."<< std::endl;
        #endif
        discoverySocket.sendPacketToServer(&sendPacket, DIRECT_TO_SERVER, &serverAddrIn);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

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
