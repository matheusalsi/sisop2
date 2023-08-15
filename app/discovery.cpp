#include "discovery.h"

void DiscoverySS::start(){
    discoverySocket.openSocket();
    discoverySocket.setSocketTimeoutMS(100);

    discoverySocket.bindSocket();
    discoverySocket.setSocketBroadcastToTrue(); 

    // Adiciona a si mesmo à tabela (para propagação)
    
    IpInfo ipInfo;
    ipInfo.hostname = g_myHostname;
    ipInfo.mac = g_myMACAddress;
    ipInfo.awake = true;

    tableManager->insertClient(g_myIP, ipInfo);

    WOLSubsystem::start();
}

void DiscoverySS::stop(){
    WOLSubsystem::stop();
    discoverySocket.closeSocket();
}

void DiscoverySS::run(){
    int exitTimeoutCount = 20; // Máximo de timeouts até desistir de notificar saída
    int discoveryFindTimeout = 50;

    while(isRunning()){
        packet recvPacket, sendPacket;

        // Lida com eleições
        if(g_electionHappening){

            continue;
        }


        if(g_isManager){ // Server - os pacotes de sleep service discovery e sleep service exit
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
                
                // Retorna para o cliente pacote informando que este é o manager
                // Não é mais necessário enviar junto a esse packet a informação do manager, pois
                // a tabela inteira será enviada
                sendPacket.type = SLEEP_SERVICE_DISCOVERY | SLEEP_SERVICE_DISCOVERY_FIND | ACKNOWLEDGE;
                // std::string packetPayload = g_myHostname + "&" + g_myMAC;
                // strcpy(sendPacket._payload, packetPayload.c_str());

                discoverySocket.sendPacket(sendPacket, DIRECT_TO_IP, clientPort, &clientIpStr);

                // Ignora duplicatas
                if(tableManager->getKnownIps()->count(clientIpStr)){
                    continue;
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
                std::string packetPayload = g_myHostname + "&" + g_myMACAddress;
                strcpy(sendPacket._payload, packetPayload.c_str());                
                #ifdef DEBUG
                std::clog << "DISCOVERY: ";
                std::clog << "Enviando packet de procura..." << std::endl;
                #endif
                
                if (!discoverySocket.sendPacket(sendPacket, BROADCAST, DISCOVERY_PORT)){
                }

                u_int16_t managerPort; // DISCOVERY_PORT
                // Ip retornado
                std::string receivedManagerIPStr;
                // Espera resposta do servidor/manager        
                if(!discoverySocket.receivePacket(recvPacket, managerPort, receivedManagerIPStr)){
                    discoveryFindTimeout--;
                    // Se houve timeout, tenta eleição
                    g_electionHappening = true;
                    electionLogger.log("Timeout em discovery, iniciando eleição.");
                    discoveryFindTimeout = 0;
                    continue;
                }
                discoveryFindTimeout = 0;
                
                #ifdef DEBUG
                // Checa se é resposta do manager
                if(recvPacket.type == (SLEEP_SERVICE_DISCOVERY | SLEEP_SERVICE_DISCOVERY_FIND | ACKNOWLEDGE)){
                    std::clog << "DISCOVERY: ";
                    std::clog << "Recebi um pacote do manager de confirmação que eu entrei!" << std::endl;
                }
                #endif

                // Notifica que esse IP é o do manager
                tableManager->setManagerIP(receivedManagerIPStr);

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

    tableManager->insertClient(ipStr, ipInfo);      
}
