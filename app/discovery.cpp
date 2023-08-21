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

    tableManager->insertClient(g_myIP, ipInfo);

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

void DiscoverySS::runAsManager(){
    std::string clientIpStr;
    uint16_t clientPort;
    packet recvPacket, sendPacket;

    // Fica esperando por pacotes de algum client       
    if(!discoverySocket.receivePacket(recvPacket, clientPort, clientIpStr)){
        return;
    } 

    // Checa o tipo de pacote: Adicionar ao sistema ou retirar do sistema
    if(recvPacket.type == (SLEEP_SERVICE_DISCOVERY | SLEEP_SERVICE_DISCOVERY_FIND)){
        std::string macAndHostnameClient;
        macAndHostnameClient = recvPacket._payload;

        // Retorna para o cliente pacote informando que este é o manager
        // Não é mais necessário enviar junto a esse packet a informação do manager, pois
        // a tabela inteira será enviada
        sendPacket.type = SLEEP_SERVICE_DISCOVERY | SLEEP_SERVICE_DISCOVERY_FIND | ACKNOWLEDGE;
        // std::string packetPayload = getHostname() + "&" + getMACAddress();
        // strcpy(sendPacket._payload, packetPayload.c_str());

        discoverySocket.sendPacket(sendPacket, DIRECT_TO_IP, clientPort, &clientIpStr);

        // Ignora duplicatas
        if(tableManager->getKnownIps()->count(clientIpStr)){
            return;
        }

        // Envia toda a tabela para o cliente (replica)
        std::string logMsg = std::string("Enviando tabela para ") + clientIpStr;
        logger.log(logMsg);
        
        tableManager->sendTableToIP(clientIpStr);

        // Adiciona cliente à tabela
        // Envia mensagem para o gerenciamento para adicionar o cliente à tabela
        prepareAndSendToTable(macAndHostnameClient, clientIpStr);
    }
    else if(recvPacket.type == (SLEEP_SERVICE_DISCOVERY | SLEEP_SERVICE_DISCOVERY_EXIT)){ 
        
        // Retorna para o cliente pacote confirmando que ele recebeu o pacote de descoberta
        sendPacket.type = SLEEP_SERVICE_DISCOVERY | SLEEP_SERVICE_DISCOVERY_EXIT | ACKNOWLEDGE;
        discoverySocket.sendPacket(sendPacket, DIRECT_TO_IP, clientPort, &clientIpStr);

        // Remove da tabela
        tableManager->removeClient(clientIpStr);
    }    
}

void DiscoverySS::runAsClient(){
    std::string clientIpStr;
    uint16_t clientPort;
    packet recvPacket, sendPacket;

    if(g_exiting){ // Verifica se é necessário mandar packet informando saída do sistema
        // O cliente já sabe qual o endereço do servidor
        discoverySocket.setSocketBroadcastToFalse(); 
        u_int16_t managerPort; // DISCOVERY_PORT

        // Envia pacote
        packet sendPacket;
        sendPacket.type = SLEEP_SERVICE_DISCOVERY | SLEEP_SERVICE_DISCOVERY_EXIT;

        // IP do manager armazenado na tabela
        // Garante que o manager que recebe "EXIT" é o atual (após possíveis eleições)
        std::string managerIP = tableManager->getManagerIP();

        if (!discoverySocket.sendPacket(sendPacket, DIRECT_TO_IP, DISCOVERY_PORT, &managerIP)){
            logger.log("Não foi possível enviar o pacote de saída");
        }

        // Espera resposta do servidor
        int nTimeouts = 0;
        while(isRunning()){
            // TO-DO: USAR DEFINE
            if(nTimeouts > 10){
                std::cout << "Não foi possível contatar o manager sobre a saída (excesso de timeouts)" << std::endl;
            }
            if(!discoverySocket.receivePacket(recvPacket, managerPort, managerIP)){
                nTimeouts++;
            }
            else{
                break;
            }
        }             

        // Checa se é resposta do manager
        if(recvPacket.type == (SLEEP_SERVICE_DISCOVERY | SLEEP_SERVICE_DISCOVERY_EXIT | ACKNOWLEDGE)){
            std::clog << "DISCOVERY: ";
            std::clog << "Recebi um pacote do manager de confirmação da saída!" << std::endl;
            setRunning(false);
        }
        // Senão, era pacote aleatório
        // TO-DO: melhorar essa parte

    }
    else if (!g_foundManager){ // Busca o manager
        packet sendPacket;
        sendPacket.type = SLEEP_SERVICE_DISCOVERY | SLEEP_SERVICE_DISCOVERY_FIND;
        std::string packetPayload = getHostname() + "&" + getMACAddress();
        strcpy(sendPacket._payload, packetPayload.c_str());                
        
        u_int16_t managerPort; // DISCOVERY_PORT
        // Ip retornado
        std::string receivedManagerIPStr;

        // Fica tentando enviar o pacote, entrando em modo de eleição se demorar muito
        int nTimeout = 0;
        while(isRunning()){
            // TO-DO: USAR DEFINE
            if(nTimeout > 20){
                g_electionHappening = true;
                return;
            }
            
            if (!discoverySocket.sendPacket(sendPacket, BROADCAST, DISCOVERY_PORT)){
                logger.log("Não foi possível enviar o pacote de procura");
                return;
            }
            // Loop de respostas, necessário para lidar com o packet que enviamos a nós mesmo no
            // broadcast
            while(isRunning()){
                if(!discoverySocket.receivePacket(recvPacket, managerPort, receivedManagerIPStr)){
                    nTimeout++;
                    break; 
                }
                // Nosso próprio packet
                else if(receivedManagerIPStr == g_myIP){
                    continue;
                }
                // Checamos se é o pacote correto
                else if(recvPacket.type == (SLEEP_SERVICE_DISCOVERY | SLEEP_SERVICE_DISCOVERY_FIND | ACKNOWLEDGE)){
                    logger.log("DISCOVERY: Recebi um pacote do manager de confirmação que eu entrei!");
                    // Notifica que esse IP é o do manager
                    tableManager->setManagerIP(receivedManagerIPStr);
                    g_foundManager = true;
                    return;
                }
            }

        }

    }   
}

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

    tableManager->insertClient(ipStr, ipInfo);      
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
