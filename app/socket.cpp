#include "socket.h"

Socket::Socket(int port){
    this->port = port;
};

void Socket::createSocket(){
    if((this->socketFD = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
        std::string errorMsg = "Não foi possível obter o socket na porta " + std::to_string(port);
        throw std::runtime_error(errorMsg);
    }
}

void Socket::setServerInfo(){
    serverAddrIn.sin_family = AF_INET;
    serverAddrIn.sin_port = htons(port);
    serverAddrIn.sin_addr.s_addr = debug ? htonl(INADDR_LOOPBACK) : INADDR_ANY; // Endereço utilizado para bind
    serverLen = sizeof(struct sockaddr_in);
}

void Socket::oldOpenSocket(){
    createSocket();
    setServerInfo();
}

void Socket::setAddrInInfo(struct sockaddr_in& sockAddrIn, struct in_addr addrIn){
    sockAddrIn.sin_family = AF_INET;
    sockAddrIn.sin_port = htons(port);
    sockAddrIn.sin_addr = addrIn;
}

void Socket::bindSocket(){
    struct sockaddr_in serverSockAddrIn;
    struct in_addr serverAddrIn;
    serverAddrIn.s_addr = INADDR_ANY;

    setAddrInInfo(serverSockAddrIn, serverAddrIn);

    bzero(&(serverSockAddrIn.sin_zero), 8);
    if (bind(socketFD, (struct sockaddr *) &serverSockAddrIn, sizeof(struct sockaddr)) < 0){
        std::string errorMsg = "Erro com o bind do socket na porta " + std::to_string(port);
        throw std::runtime_error(errorMsg);
    } 
}

void Socket::openSocket(){
    if((this->socketFD = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
        std::string errorMsg = "Não foi possível obter o socket na porta " + std::to_string(port);
        throw std::runtime_error(errorMsg);
    }
}


void Socket::closeSocket(){
    close(this->socketFD);
}

void Socket::oldBindSocket(){
    bzero(&(serverAddrIn.sin_zero), 8);
    if (bind(socketFD, (struct sockaddr *) &serverAddrIn, sizeof(struct sockaddr)) < 0){
        std::string errorMsg = "Erro com o bind do socket na porta " + std::to_string(port);
        throw std::runtime_error(errorMsg);
    } 
}

int Socket::getSocketDescriptor(){
    return socketFD;
}

struct in_addr Socket::getServerBinaryNetworkAddress(){
    return serverAddrIn.sin_addr;
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

int Socket::sendPacket(struct packet& packet, int send_type, char* ipDst){
    struct sockaddr_in dstSockAddrIn;
    in_addr dstAddrIn;
    
    if (send_type == DIRECT_TO_IP){
        if (ipDst == NULL){
            std::string errorMsg = "Erro: IP de destino não definido na porta " + std::to_string(port);
            throw std::runtime_error(errorMsg);
        }
        else{
            // Converte o IP de destino para o endereço binário de rede
            inet_pton(AF_INET, ipDst, &dstAddrIn);
        }
    }
    else if (send_type == BROADCAST){
        dstAddrIn.s_addr = INADDR_BROADCAST;
    }
    else if (send_type == LOOPBACK){
        dstAddrIn.s_addr = htonl(INADDR_LOOPBACK);
    }

    setAddrInInfo(dstSockAddrIn, dstAddrIn);

    int n = sendto(socketFD, &packet, sizeof(struct packet), 0, (const struct sockaddr *) &dstSockAddrIn, sizeof(struct sockaddr_in));
    return n;
}

int Socket::receivePacket(struct packet& packet, char* ipSrc){
    sockaddr_in srcSockAddrIn;
    socklen_t srcLen = sizeof(srcSockAddrIn);
    int n = recvfrom(socketFD, &packet, sizeof(struct packet), 0, (struct sockaddr *) &srcSockAddrIn, &srcLen);

    // Recebeu com sucesso o pacote
    if (n > 0){
        // Converte o endereço binário de rede para o IP de quem enviou
        inet_ntop(AF_INET, &(srcSockAddrIn.sin_addr), ipSrc, INET_ADDRSTRLEN);
    }
    return n;
}

// ANTIGAS
// SERVIDOR
void Socket::oldsendPacketToClient(struct packet* sendPacketServer, struct sockaddr_in clientAddrIn){
    int n = sendto(socketFD, sendPacketServer, sizeof(struct packet), 0, (const struct sockaddr *) &clientAddrIn, sizeof(struct sockaddr_in));
    if (n < 0){
        std::string errorMsg = "Erro ao enviar um pacote para o servidor na porta " + std::to_string(port);
        throw std::runtime_error(errorMsg);
    }

}

bool Socket::oldreceivePacketFromClients(struct packet* recvPacketServer, sockaddr_in& clientAddrIn){
    socklen_t clientLen = sizeof(clientAddrIn);
    int n = recvfrom(socketFD, recvPacketServer, sizeof(struct packet), 0, (struct sockaddr *) &clientAddrIn, &clientLen);
    if (n < 0){
        if(errno == EAGAIN || errno == EWOULDBLOCK){
            return false;
        }

        std::string errorMsg = "Erro ao receber um pacote do cliente na porta " + std::to_string(port);
        throw std::runtime_error(errorMsg);
    }
 
    return true;

} 

// CLIENTE
void Socket::oldsendPacketToServer(struct packet* sendPacketClient, int type, struct sockaddr_in* serverAddrInPassed){
    if (type == BROADCAST){
        serverAddrIn.sin_addr.s_addr = INADDR_BROADCAST;
    }
    else if (type == LOOPBACK){
        serverAddrIn.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    }
    else if (type == DIRECT_TO_IP){
        if (serverAddrInPassed == NULL){
            std::string errorMsg = "Erro: Servidor não definido na porta " + std::to_string(port);
            throw std::runtime_error(errorMsg);
        }
       serverAddrIn.sin_addr = *((struct in_addr *)&serverAddrInPassed->sin_addr);
    }

    int n = sendto(socketFD, sendPacketClient, sizeof(struct packet), 0, (const struct sockaddr *) &serverAddrIn, sizeof(struct sockaddr_in));
    if (n < 0){
        std::string errorMsg = "Erro ao enviar um pacote para o cliente na porta " + std::to_string(port);
        throw std::runtime_error(errorMsg);
    }
}

bool Socket::oldreceivePacketFromServer(struct packet* recvPacketClient){
    int n = recvfrom(socketFD, recvPacketClient, sizeof(struct packet), 0, (struct sockaddr *) &serverAddrIn, &serverLen);
    if (n < 0){
        if(errno == EAGAIN || errno == EWOULDBLOCK){
            return false;
        }


        std::string errorMsg = "Erro ao receber um pacote do servidor na porta " + std::to_string(port);
        throw std::runtime_error(errorMsg);
    }
    return true;
}
