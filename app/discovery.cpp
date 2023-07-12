#include "discovery.h"

void DiscoverySS::start(){
    socket.openSocket();

    if(isManager()){ // Server
        socket.bindSocket();
    }
    else{ // Client
        socket.setSocketBroadcastToTrue(); // precisa?
    }
    foundManager = false;
    WOLSubsystem::start();
}

void DiscoverySS::stop(){
    socket.closeSocket();
}


void DiscoverySS::run(){
    while(isRunning()){
        packet recvPacket, sendPacket;
        if(isManager()){ // Server - recebe esses pacotes
            #ifdef DEBUG
            std::cout << "Estou esperando um packet em " << DISCOVERY_PORT << std::endl;
            #endif

            // Fica esperando por pacotes de algum cliente
            sockaddr_in clientAddrIn;
            clientAddrIn = socket.receivePacketFromClients(&recvPacket);

            #ifdef DEBUG
            char buffer[INET_ADDRSTRLEN];
            inet_ntop( AF_INET, &clientAddrIn.sin_addr, buffer, sizeof( buffer ));
            std::cout << "Recebi um pacote de " << buffer << "!" << std::endl;
            #endif

            // Checa o tipo de pacote: Adicionar ao sistema ou retirar do sistema
            if(recvPacket.type == (SLEEP_SERVICE_DISCOVERY | SLEEP_SERVICE_DISCOVERY_FIND)){
                

                #ifdef DEBUG
                std::cout << "Estou respondendo o cliente" << std::endl;
                #endif
                
                // Retorna para o cliente pacote informando que este é o manager
                sendPacket.type = SLEEP_SERVICE_DISCOVERY | SLEEP_SERVICE_DISCOVERY_FIND | ACKNOWLEDGE;
                
                socket.sendPacketToClient(&sendPacket, clientAddrIn);
    
            }
            else if(recvPacket.type == (SLEEP_SERVICE_DISCOVERY | SLEEP_SERVICE_DISCOVERY_EXIT)){ // Exit
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


                while(true){
                    socket.receivePacketFromServer(&recvPacket);
                    // Checa se é resposta do manager
                    if(recvPacket.type == (SLEEP_SERVICE_DISCOVERY | SLEEP_SERVICE_DISCOVERY_FIND | ACKNOWLEDGE)){
                        break;
                    }

                    // Eu creio que esse caso não tem como acontecer? 
                    // Porque a gente tem salvo as informações do servidor e elas não vão mudar
                    // E não tem como um participante mandar um pacote pra um IP além do IP do servidor nessa porta

                    // #ifdef DEBUG
                    // char buffer[INET_ADDRSTRLEN];
                    // inet_ntop( AF_INET, &from.sin_addr, buffer, sizeof( buffer ));
                    // std::cout << "Recebi packet de " << buffer << ", mas não é o manager!";
                    // #endif
                }

                #ifdef DEBUG
                char buffer[INET_ADDRSTRLEN];
                struct in_addr serverBinaryNetworkAddr = socket.getServerBinaryNetworkAddress();
                inet_ntop( AF_INET, &serverBinaryNetworkAddr, buffer, sizeof( buffer ));
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
    packet sendPacket;
    sendPacket.type = SLEEP_SERVICE_DISCOVERY | SLEEP_SERVICE_DISCOVERY_FIND;

    // Manda o pacote de discovery enquanto não recebe confirmação
    while(!foundManager && isRunning()){
        #ifdef DEBUG
        std::cout << "Enviando packet de procura..."<< std::endl;
        #endif
        socket.sendPacketToServer(&sendPacket);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

}

std::string DiscoverySS::getHostname(){
    return std::string("TESTE");
}

std::string DiscoverySS::getMACAddress(){
    return std::string("00:00:00:00:00:00");
}