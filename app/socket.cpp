#include "socket.h"

Socket::Socket(int port, bool debug=false){
    this->port = port;
    this->debug = debug; // Utilizado para testar na mesma máquina passando pacotes em LOOPBACK
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

void Socket::openSocket(){
    createSocket();
    setServerInfo();
}

void Socket::closeSocket(){
    close(this->socketFD);
}

void Socket::bindSocket(){
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

// SERVIDOR
void Socket::sendPacketToClient(struct packet* sendPacketServer, struct sockaddr_in clientAddrIn){
    int n = sendto(socketFD, sendPacketServer, sizeof(struct packet), 0, (const struct sockaddr *) &clientAddrIn, sizeof(struct sockaddr_in));
    if (n < 0){
        std::string errorMsg = "Erro ao enviar um pacote para o servidor na porta " + std::to_string(port);
        throw std::runtime_error(errorMsg);
    }

}

bool Socket::receivePacketFromClients(struct packet* recvPacketServer, sockaddr_in& clientAddrIn){
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
void Socket::sendPacketToServer(struct packet* sendPacketClient, int type, struct sockaddr_in* serverAddrInPassed){
    if (type == BROADCAST){
        serverAddrIn.sin_addr.s_addr = INADDR_BROADCAST;
    }
    else if (type == LOOPBACK){
        serverAddrIn.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    }
    else if (type == DIRECT_TO_SERVER){
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

bool Socket::receivePacketFromServer(struct packet* recvPacketClient){
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
