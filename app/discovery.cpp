#include "discovery.h"

void DiscoverySS::start(){
    if((discoverySocketFD = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
        throw std::runtime_error("DiscoverySS: não foi possível obter socket");
    }


    if(isManager()){ // Server
        discoverySocketServerAddrIn.sin_family = AF_INET;
        discoverySocketServerAddrIn.sin_port = htons(DISCOVERY_PORT);
        discoverySocketServerAddrIn.sin_addr.s_addr = INADDR_ANY;
        bzero(&(discoverySocketServerAddrIn.sin_zero), 8);

        if (bind(discoverySocketFD, (struct sockaddr *) &discoverySocketServerAddrIn, sizeof(struct sockaddr)) < 0){
            throw std::runtime_error("DiscoverySS: erro com o bind do socket");
        } 

    }
    else{ // Client
        discoverySocketServerAddrIn.sin_family = AF_INET;
        discoverySocketServerAddrIn.sin_port = htons(DISCOVERY_PORT);
        discoverySocketServerAddrIn.sin_addr.s_addr = INADDR_BROADCAST;
    }

    cli_len = sizeof(struct sockaddr_in);
    foundManager = false;

}

void DiscoverySS::stop(){
    close(discoverySocketFD);
}


void DiscoverySS::run(){
    int n;
    while(isRunning()){

        packet recv_packet, send_packet;
        if(isManager()){ // Server - recebe esses pacotes

            // Fica esperando para ver se o cliente enviou pacote de saída
            n = recvfrom(discoverySocketFD, &recv_packet, sizeof(packet), 0, (struct sockaddr *) &discoverySocketClientAddrIn, &cli_len);
            if (n < 0){ 
                throw std::runtime_error("DiscoverySS: erro com recvfrom");
            } 

            // Checa o tipo de pacote: Adicionar ao sistema ou retirar do sistema
            if(recv_packet.type == (SLEEP_SERVICE_DISCOVERY | SLEEP_SERVICE_DISCOVERY_FIND)){
                send_packet.type = SLEEP_SERVICE_DISCOVERY | SLEEP_SERVICE_DISCOVERY_FIND | ACKNOWLEDGE;
                // Retorna para o cliente pacote informando que este é o manager
                n = sendto(discoverySocketFD, &send_packet, sizeof(packet), 0, (const struct sockaddr *) &discoverySocketServerAddrIn, sizeof(struct sockaddr_in));
    
            }
            else if(recv_packet.type == (SLEEP_SERVICE_DISCOVERY | SLEEP_SERVICE_DISCOVERY_EXIT)){ // Exit
                std::cout << "AWDWADWADWADWA" << std::endl;
            }
            
            char buffer[INET_ADDRSTRLEN];
            inet_ntop( AF_INET, &discoverySocketClientAddrIn.sin_addr, buffer, sizeof( buffer ));
            std::cout << buffer << std::endl;

            

    
        }
        else{ // Client - envia esses pacotes
            // Checa se já foi encontrado um manager
            if(foundManager){ // Verifica se é necessário mandar packet informando saída do sistema
                continue;
            }
            else{ // Busca o manager
                std::thread packetSenderThread(&DiscoverySS::sendSleepDiscoverPackets, this);
                
                while(true){
                    n = recvfrom(discoverySocketFD, &recv_packet, sizeof(packet), 0, (struct sockaddr *) &discoverySocketClientAddrIn, &cli_len);
                    // Checa se é resposta do manager
                    if(recv_packet.type == (SLEEP_SERVICE_DISCOVERY | SLEEP_SERVICE_DISCOVERY_FIND | ACKNOWLEDGE)){
                        break;
                    }
                }

                foundManager = true; // Seta como true, acaba encerrando a thread "packet sender"
                packetSenderThread.join(); // Espera a thread encerrar

                // ENVIA INFORMAÇÕES DO MANAGER PARA O BIRIRIRI
                char buffer[INET_ADDRSTRLEN];
                inet_ntop( AF_INET, &discoverySocketClientAddrIn.sin_addr, buffer, sizeof( buffer ));
                std::cout << buffer << std::endl;

            }

        }

    }
};

void DiscoverySS::sendSleepDiscoverPackets(){
    int n;

    packet send_packet;
    send_packet.type = SLEEP_SERVICE_DISCOVERY | SLEEP_SERVICE_DISCOVERY_FIND;

    // Manda o pacote de discovery enquanto não recebe confirmação
    while(!foundManager && isRunning()){
        n = sendto(discoverySocketFD, &send_packet, sizeof(packet), 0, (const struct sockaddr *) &discoverySocketServerAddrIn, sizeof(struct sockaddr_in));
        if (n < 0){
            throw std::runtime_error("DiscoverySS: erro com sendto");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

}

std::string DiscoverySS::getHostname(){
    return std::string("TESTE");
}

std::string DiscoverySS::getMACAddress(){
    return std::string("00:00:00:00:00:00");
}