#include "discovery.h"

void DiscoverySS::start(){
    if((discoverySocketFD = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
        throw std::runtime_error("DiscoverySS: não foi possível obter socket");
    }

    if(isManager()){ // Server
        
        sockaddr_in svr_in;
        svr_in.sin_family = AF_INET;
        svr_in.sin_port = htons(DISCOVERY_PORT);
        //svr_in.sin_addr.s_addr = INADDR_ANY;
        svr_in.sin_addr.s_addr = htonl(INADDR_LOOPBACK);; // PLACEHOLDER
        

        if (bind(discoverySocketFD, (struct sockaddr *) &svr_in, sizeof(struct sockaddr)) < 0){
            throw std::runtime_error("DiscoverySS: erro com o bind do socket");
        } 

    }
    else{ // Client
        int yes = 1;
        setsockopt(discoverySocketFD, SOL_SOCKET, SO_BROADCAST, (char*)&yes, sizeof(yes));
    }

    foundManager = false;

    WOLSubsystem::start();

}

void DiscoverySS::stop(){
    close(discoverySocketFD);
}


void DiscoverySS::run(){
    int n;
    while(isRunning()){

        packet recv_packet, send_packet;
        if(isManager()){ // Server - recebe esses pacotes

            #ifdef DEBUG
            std::cout << "Estou esperando um packet em " << DISCOVERY_PORT << std::endl;
            #endif


            // Fica esperando por pacotes de algum cliente
            sockaddr_in cli_in;
            socklen_t cli_len = sizeof(cli_in);
            n = recvfrom(discoverySocketFD, &recv_packet, sizeof(packet), 0, (struct sockaddr *) &cli_in, &cli_len);
            if (n < 0){ 
                throw std::runtime_error("DiscoverySS: erro com recvfrom");
            }

            #ifdef DEBUG
            char buffer[INET_ADDRSTRLEN];
            inet_ntop( AF_INET, &cli_in.sin_addr, buffer, sizeof( buffer ));
            std::cout << "Recebi um pacote de " << buffer << "!" << std::endl;
            #endif

            // Checa o tipo de pacote: Adicionar ao sistema ou retirar do sistema
            if(recv_packet.type == (SLEEP_SERVICE_DISCOVERY | SLEEP_SERVICE_DISCOVERY_FIND)){
                

                #ifdef DEBUG
                std::cout << "Estou respondendo o cliente" << std::endl;
                #endif
                
                // Retorna para o cliente pacote informando que este é o manager
                send_packet.type = SLEEP_SERVICE_DISCOVERY | SLEEP_SERVICE_DISCOVERY_FIND | ACKNOWLEDGE;
                
                n = sendto(discoverySocketFD, &send_packet, sizeof(packet), 0, (const struct sockaddr *) &cli_in, sizeof(struct sockaddr_in));
    
            }
            else if(recv_packet.type == (SLEEP_SERVICE_DISCOVERY | SLEEP_SERVICE_DISCOVERY_EXIT)){ // Exit
                std::cout << "AWDWADWADWADWA" << std::endl;
            }
    
        }
        else{ // Client - envia esses pacotes
            // Checa se já foi encontrado um manager
            if(foundManager){ // Verifica se é necessário mandar packet informando saída do sistema
                continue;
            }
            else{ // Busca o manager
                // Thread que envia packets ao servidor
                std::thread packetSenderThread(&DiscoverySS::sendSleepDiscoverPackets, this);
                
                // Espera resposta do servidor
                sockaddr_in from;
                socklen_t from_len = sizeof(from);

                while(true){
                    n = recvfrom(discoverySocketFD, &recv_packet, sizeof(packet), 0, (struct sockaddr *) &from, &from_len);
                    // Checa se é resposta do manager
                    if(recv_packet.type == (SLEEP_SERVICE_DISCOVERY | SLEEP_SERVICE_DISCOVERY_FIND | ACKNOWLEDGE)){
                        break;
                    }
                    #ifdef DEBUG
                    char buffer[INET_ADDRSTRLEN];
                    inet_ntop( AF_INET, &from.sin_addr, buffer, sizeof( buffer ));
                    std::cout << "Recebi packet de " << buffer << ", mas não é o manager!";
                    #endif
                }

                #ifdef DEBUG
                char buffer[INET_ADDRSTRLEN];
                inet_ntop( AF_INET, &from.sin_addr, buffer, sizeof( buffer ));
                std::cout << "Recebi um pacote do gerenciador " << "(" << buffer << ")" << "!" << std::endl;
                #endif

                foundManager = true; // Seta como true, acaba encerrando a thread "packet sender"
                packetSenderThread.join(); // Espera a thread encerrar

                // GUARDA INFORMAÇÕES DO MANAGER (TO-DO)

            }

        }

    }
};

void DiscoverySS::sendSleepDiscoverPackets(){
    int n;

    packet send_packet;
    send_packet.type = SLEEP_SERVICE_DISCOVERY | SLEEP_SERVICE_DISCOVERY_FIND;

    sockaddr_in svr_in;
    svr_in.sin_family = AF_INET;
    svr_in.sin_port = htons(DISCOVERY_PORT);
    //svr_in.sin_addr.s_addr = INADDR_BROADCAST;
    svr_in.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // PLACEHOLDER

    // Manda o pacote de discovery enquanto não recebe confirmação
    while(!foundManager && isRunning()){
        #ifdef DEBUG
        std::cout << "Enviando packet de procura..."<< std::endl;
        #endif

        n = sendto(discoverySocketFD, &send_packet, sizeof(packet), 0, (const struct sockaddr *) &svr_in, sizeof(struct sockaddr_in));
        if (n < 0){
            throw std::runtime_error("DiscoverySS: erro com sendto");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

}

std::string DiscoverySS::getHostname(){
    return std::string("TESTE");
}

std::string DiscoverySS::getMACAddress(){
    return std::string("00:00:00:00:00:00");
}