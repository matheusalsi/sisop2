#include "discovery.h"

void DiscoverySS::start(){
    if(discoverySocketFD = socket(AF_INET, SOCK_DGRAM, 0)){
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

        if(isManager()){ // Server - recebe esses pacotes
            // n = recvfrom(discoverySocketFD, buf, 256, 0, (struct sockaddr *) &discoverySocketClientAddrIn, &cli_len);
            // if (n < 0){
            //     throw std::runtime_error("DiscoverySS: erro com recvfrom");
            // } 
            // // Retorna para o cliente pacote informando que este é o manager
            // n = sendto(discoverySocketFD, buf, strlen(buffer), 0, (const struct sockaddr *) &discoverySocketServerAddrIn, sizeof(struct sockaddr_in));

            // // Fica esperando para ver se o cliente enviou pacote de saída

    
        }
        else{ // Client - envia esses pacotes
            // Checa se já foi encontrado um manager
            // if(foundManager){ // Verifica se é necessário mandar packet informando saída do sistema
            //     continue;
            // }
            // else{ // Busca o manager
            //     std::thread packetSenderThread(DiscoverySS::sendSleepDiscoverPackets, this);
            //     n = recvfrom(discoverySocketFD, buf, 256, 0, (struct sockaddr *) &discoverySocketClientAddrIn, &cli_len);
            //     foundManager = true; // Seta como true, acaba encerrando a thread "packet sender"
            //     packetSenderThread.join(); // Espera a thread encerrar

            //     // ENVIA INFORMAÇÕES DO MANAGER PARA O BIRIRIRI

            // }

        }

    }
};

// void DiscoverySS::sendSleepDiscoverPackets(){
//     int n;
//     // Manda o pacote de discovery enquanto não recebe confirmação
//     while(!foundManager){
//         n = sendto(discoverySocketFD, buffer, strlen(buffer), 0, (const struct sockaddr *) &discoverySocketServerAddrIn, sizeof(struct sockaddr_in));
//         if (n < 0){
//             throw std::runtime_error("DiscoverySS: erro com sendto");
//         }
//         std::this_thread::sleep_for(std::chrono::milliseconds(100));
//     }

// }