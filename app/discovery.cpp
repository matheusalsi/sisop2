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
    int exitTimeoutCount = 20; // Máximo de timeouts até desistir de notificar saída

    while(isRunning()){
        packet recvPacket, sendPacket;

        if(isManager()){ // Server - os pacotes de sleep service discovery e sleep service exit
            char clientIpStr[INET_ADDRSTRLEN];

            // Fica esperando por pacotes de algum client       
            if(discoverySocket.receivePacket(recvPacket, clientIpStr) < 0){
                continue;
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

                discoverySocket.sendPacket(sendPacket, DIRECT_TO_IP, clientIpStr);

                // Adiciona cliente à tabela
                // Envia mensagem para o gerenciamento para adicionar o cliente à tabela
                std::string message;
                
                // Info
                IpInfo ipInfo;
                // Separa hostname e mac da string contida no packet
                std::string hostname, mac;
                hostname = macAndHostnameClient.substr(0, macAndHostnameClient.find('&'));
                mac = macAndHostnameClient.substr(macAndHostnameClient.find('&')+1);
                ipInfo.hostname = hostname;
                ipInfo.mac = mac;
                ipInfo.awake = true; // Por default, awake

                tableManager->insertClient(clientIpStr, ipInfo);
            }

            else if(recvPacket.type == (SLEEP_SERVICE_DISCOVERY | SLEEP_SERVICE_DISCOVERY_EXIT)){ 
                #ifdef DEBUG
                std::clog << "DISCOVERY: ";
                std::clog << "Estou respondendo o cliente " << buffer << " que quer sair" << std::endl;
                #endif
                
                // Retorna para o cliente pacote confirmando que ele recebeu o pacote de descoberta
                sendPacket.type = SLEEP_SERVICE_DISCOVERY | SLEEP_SERVICE_DISCOVERY_EXIT | ACKNOWLEDGE;
                discoverySocket.sendPacket(sendPacket, DIRECT_TO_IP, clientIpStr);

                // Remove da tabela
                tableManager->removeClient(clientIpStr);
            }
    
        }
        else{ // Client - envia os pacotes de sleep service discovery e sleep service exit
            if (!foundManager){ // Busca o manager
                // Envia pacote
                sendSleepDiscoverPackets();
                // Espera resposta do servidor        
                if(discoverySocket.receivePacket(recvPacket, managerIpStr) < 0){
                    continue;
                }
                
                #ifdef DEBUG
                // Checa se é resposta do manager
                if(recvPacket.type == (SLEEP_SERVICE_DISCOVERY | SLEEP_SERVICE_DISCOVERY_FIND | ACKNOWLEDGE)){
                    std::clog << "DISCOVERY: ";
                    std::clog << "Recebi um pacote do manager de confirmação que eu entrei!" << std::endl;
                }
                #endif

                // TO-DO: Duplicado. Ver como resolver isso.  
                // Adiciona o manager à tabela
                std::string macAndHostnameManager;
                macAndHostnameManager = recvPacket._payload;

                std::string message;
                IpInfo ipInfo;

                // Separa hostname e mac da string contida no packet
                std::string hostname, mac;
                hostname = macAndHostnameManager.substr(0, macAndHostnameManager.find('&'));
                mac = macAndHostnameManager.substr(macAndHostnameManager.find('&')+1);
                ipInfo.hostname = hostname;
                ipInfo.mac = mac;
                ipInfo.awake = true; // Por default, awake

                std::cout << managerIpStr << std::endl;
                tableManager->insertClient(managerIpStr, ipInfo);
                
                foundManager = true;
            }
                        
            if(g_exiting){ // Verifica se é necessário mandar packet informando saída do sistema
                // O cliente já sabe qual o endereço do servidor
                discoverySocket.setSocketBroadcastToFalse(); 

                // Envia pacote
                sendSleepExitPackets();

                // Espera resposta do servidor               
                if(discoverySocket.receivePacket(recvPacket, managerIpStr) < 0){
                // Timeout
                    if(exitTimeoutCount-- > 0){
                        continue;
                    }
                    else{
                        std::cout << "Não foi possível contatar o manager sobre a saída (excesso de timeouts)" << std::endl;
                    }
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
                continue;
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
    
    discoverySocket.sendPacket(sendPacket, BROADCAST, NULL);
}

void DiscoverySS::sendSleepExitPackets(){
    packet sendPacket;
    sendPacket.type = SLEEP_SERVICE_DISCOVERY | SLEEP_SERVICE_DISCOVERY_EXIT;

    #ifdef DEBUG
    std::clog << "DISCOVERY: ";
    std::clog << "Enviando packet de saída..."<< std::endl;
    #endif
  
    discoverySocket.sendPacket(sendPacket, DIRECT_TO_IP, managerIpStr);
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