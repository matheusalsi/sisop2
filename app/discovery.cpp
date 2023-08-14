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

    // Adiciona a si mesmo à tabela (para propagação)
    
    IpInfo ipInfo;
    ipInfo.hostname = getHostname();
    ipInfo.mac = getMACAddress();
    ipInfo.awake = true;

    tableManager->insertClient(g_myIP, ipInfo, false);

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

        // Lida com eleições
        if(g_electionHappening){

            continue;
        }


        if(isManager()){ // Server - os pacotes de sleep service discovery e sleep service exit
            std::string clientIpStr;
            uint16_t clientPort;

            // Fica esperando por pacotes de algum client       
            if(!discoverySocket.receivePacket(recvPacket, clientPort, clientIpStr)){
                continue;
            } 

            #ifdef DEBUG
            std::clog << "DISCOVERY: ";
            std::clog << "Recebi um pacote de " << clientIpStr << "!" << std::endl;
            #endif

            // Checa o tipo de pacote: Adicionar ao sistema ou retirar do sistema
            if(recvPacket.type == (SLEEP_SERVICE_DISCOVERY | SLEEP_SERVICE_DISCOVERY_FIND)){
                std::string macAndHostnameClient;
                macAndHostnameClient = recvPacket._payload;

                #ifdef DEBUG
                std::clog << "DISCOVERY: ";
                std::clog << "Estou respondendo o cliente " << clientIpStr << " que quer entrar" << std::endl;
                #endif
                
                // Retorna para o cliente pacote informando que este é o manager, com suas informações de hostname e MAC
                sendPacket.type = SLEEP_SERVICE_DISCOVERY | SLEEP_SERVICE_DISCOVERY_FIND | ACKNOWLEDGE;
                std::string packetPayload = getHostname() + "&" + getMACAddress();
                strcpy(sendPacket._payload, packetPayload.c_str());

                discoverySocket.sendPacket(sendPacket, DIRECT_TO_IP, clientPort, &clientIpStr);

                // Adiciona cliente à tabela
                // Envia mensagem para o gerenciamento para adicionar o cliente à tabela
                prepareAndSendToTable(macAndHostnameClient, clientIpStr);
            }
            else if(recvPacket.type == (SLEEP_SERVICE_DISCOVERY | SLEEP_SERVICE_DISCOVERY_EXIT)){ 
                #ifdef DEBUG
                std::clog << "DISCOVERY: ";
                std::clog << "Estou respondendo o cliente " << clientIpStr << " que quer sair" << std::endl;
                #endif
                
                // Retorna para o cliente pacote confirmando que ele recebeu o pacote de descoberta
                sendPacket.type = SLEEP_SERVICE_DISCOVERY | SLEEP_SERVICE_DISCOVERY_EXIT | ACKNOWLEDGE;
                discoverySocket.sendPacket(sendPacket, DIRECT_TO_IP, clientPort, &clientIpStr);

                // Remove da tabela
                tableManager->removeClient(clientIpStr);
            }    
        }
        else{ // Client - envia os pacotes de sleep service discovery e sleep service exit
            if(g_exiting){ // Verifica se é necessário mandar packet informando saída do sistema
                // O cliente já sabe qual o endereço do servidor
                discoverySocket.setSocketBroadcastToFalse(); 
                u_int16_t managerPort; // DISCOVERY_PORT

                // Envia pacote
                packet sendPacket;
                sendPacket.type = SLEEP_SERVICE_DISCOVERY | SLEEP_SERVICE_DISCOVERY_EXIT;
                #ifdef DEBUG
                std::clog << "DISCOVERY: ";
                std::clog << "Enviando packet de saída..."<< std::endl;
                #endif
            
                // IP do manager armazenado na tabela
                // Garante que o manager que recebe "EXIT" é o atual (após possíveis eleições)
                std::string managerIP = tableManager->getManagerIP();

                if (!discoverySocket.sendPacket(sendPacket, DIRECT_TO_IP, DISCOVERY_PORT, &managerIP)){
                    #ifdef DEBUG
                    std::clog << "Não foi possível enviar o pacote de saída" << std::endl;
                    #endif
                }

                // Espera resposta do servidor               
                if(!discoverySocket.receivePacket(recvPacket, managerPort, managerIP)){
                // Timeout
                    if(exitTimeoutCount--> 0){
                        continue;
                    }
                    else{
                        std::cout << "Não foi possível contatar o manager sobre a saída (excesso de timeouts)" << std::endl;
                    }
                }

                // Checa se é resposta do manager
                #ifdef DEBUG
                if(recvPacket.type == (SLEEP_SERVICE_DISCOVERY | SLEEP_SERVICE_DISCOVERY_EXIT | ACKNOWLEDGE)){
                    std::clog << "DISCOVERY: ";
                    std::clog << "Recebi um pacote do manager de confirmação da saída!" << std::endl;
                }
                #endif
                
                // Finalmente, encerra o subsistema
                setRunning(false);
                continue;
            }
            else if (!g_foundManager){ // Busca o manager
                // Envia pacote
                packet sendPacket;

                sendPacket.type = SLEEP_SERVICE_DISCOVERY | SLEEP_SERVICE_DISCOVERY_FIND;
                std::string packetPayload = getHostname() + "&" + getMACAddress();
                strcpy(sendPacket._payload, packetPayload.c_str());                
                #ifdef DEBUG
                std::clog << "DISCOVERY: ";
                std::clog << "Enviando packet de procura..." << std::endl;
                #endif
                
                if (!discoverySocket.sendPacket(sendPacket, BROADCAST, DISCOVERY_PORT)){
                    #ifdef DEBUG
                    std::clog << "Não foi possível enviar o pacote de procura" << std::endl;
                    #endif
                }

                u_int16_t managerPort; // DISCOVERY_PORT
                // Ip retornado
                std::string receivedManagerIPStr;
                // Espera resposta do servidor/manager        
                if(!discoverySocket.receivePacket(recvPacket, managerPort, receivedManagerIPStr)){
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

                // Envia mensagem para o gerenciamento para adicionar o manager à tabela do cliente.
                prepareAndSendToTable(macAndHostnameManager, receivedManagerIPStr);        
                g_foundManager = true;
            }
        }
    }
};

void DiscoverySS::prepareAndSendToTable(std::string macAndHostname, std::string ipStr){
    // Envia mensagem para o gerenciamento para adicionar o cliente à tabela
    std::string message;   
    IpInfo ipInfo;

    // Separa hostname e mac da string contida no packet
    std::string hostname, mac;
    hostname = macAndHostname.substr(0, macAndHostname.find('&'));
    mac = macAndHostname.substr(macAndHostname.find('&')+1);
    ipInfo.hostname = hostname;
    ipInfo.mac = mac;
    ipInfo.awake = true; // Por default, awake

    // Se é um cliente, então esse ip é o do manager
    bool insertingManager = isManager() ? false : true;

    tableManager->insertClient(ipStr, ipInfo, insertingManager);      
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
