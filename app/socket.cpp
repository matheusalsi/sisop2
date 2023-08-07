#include "socket.h"

Socket::Socket(int port){
    this->port = port;
};

void Socket::openSocket(){
    if((this->socketFD = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
        std::string errorMsg = "Não foi possível obter o socket na porta " + std::to_string(port);
        throw std::runtime_error(errorMsg);
    }
}

void Socket::bindSocket(){
    struct sockaddr_in serverSockAddrIn;

    serverSockAddrIn.sin_family = AF_INET;
    serverSockAddrIn.sin_port = htons(port);
    serverSockAddrIn.sin_addr.s_addr = INADDR_ANY;
    

    bzero(&(serverSockAddrIn.sin_zero), 8);
    if (bind(socketFD, (struct sockaddr *) &serverSockAddrIn, sizeof(struct sockaddr)) < 0){
        std::string errorMsg = "Erro com o bind do socket na porta " + std::to_string(port);
        throw std::runtime_error(errorMsg);
    } 
}

void Socket::closeSocket(){
    close(this->socketFD);
}


void Socket::setSocketTimeoutMS(int timeMS){
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = timeMS * 1000;
    if (setsockopt(socketFD, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
        perror("Error");
    }
}

void Socket::setSocketBroadcastToTrue(){
    int yes = 1;
    setsockopt(socketFD, SOL_SOCKET, SO_BROADCAST, (char*)&yes, sizeof(yes));
}

void Socket::setSocketBroadcastToFalse(){
    int no = 0;
    setsockopt(socketFD, SOL_SOCKET, SO_BROADCAST, (char*)&no, sizeof(no));
}

// FUNÇÕES DE TROCAS DE PACOTES
bool Socket::sendPacket(struct packet& packet, int send_type, uint16_t portDst, std::string* ipDstStr){
    struct sockaddr_in dstSockAddrIn;

    // Seta as informações do destinatário do pacote
    dstSockAddrIn.sin_family = AF_INET;
    dstSockAddrIn.sin_port = htons(portDst);

    if (send_type == DIRECT_TO_IP){
        if (ipDstStr == NULL){
            std::string errorMsg = "Erro: IP de destino não definido na porta " + std::to_string(port);
            throw std::runtime_error(errorMsg);
        }
        else{
            char ipDst[INET_ADDRSTRLEN];
            strcpy(ipDst, ipDstStr->c_str());
            if (inet_aton(ipDst, &dstSockAddrIn.sin_addr) < 0){
                std::string errorMsg = "Erro: IP de destino inválido na porta " + std::to_string(port);
                throw std::runtime_error(errorMsg);
            }
        }
    }
    else if (send_type == BROADCAST){
        dstSockAddrIn.sin_addr.s_addr = INADDR_BROADCAST;
    }

    int n = sendto(socketFD, &packet, sizeof(struct packet), 0, (const struct sockaddr *) &dstSockAddrIn, sizeof(struct sockaddr_in));
    return n > 0;
}

bool Socket::receivePacket(struct packet& packet, uint16_t& portSrc, std::string& ipSrcStr){
    sockaddr_in srcSockAddrIn;   

    socklen_t srcLen = sizeof(srcSockAddrIn);

    int n = recvfrom(socketFD, &packet, sizeof(struct packet), 0, (struct sockaddr *) &srcSockAddrIn, &srcLen);

    // Recebeu com sucesso o pacote
    if (n > 0){
        // Converte o endereço binário de rede para o IP de quem enviou
        ipSrcStr = inet_ntoa(srcSockAddrIn.sin_addr);
        portSrc = ntohs(srcSockAddrIn.sin_port);
    }
    return n > 0;
}
