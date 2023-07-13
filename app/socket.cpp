#include "socket.h"

Socket::Socket(int port, bool debug=false){
    this->port = port;
    this->debug = debug; // Utilizado para testar na mesma máquina passando pacotes em LOOPBACK
};

// Cria um socket, e define as informações que serão utilizadas para bindar o servidor na porta 
void Socket::openSocket(){
    createSocket();
    setServerBindInfo();
}

void Socket::closeSocket(){
    close(this->socketFD);
}

// Binda o servidor na porta especificada
void Socket::bindSocket(){
    bzero(&(serverAddrIn.sin_zero), 8);
    if (bind(socketFD, (struct sockaddr *) &serverAddrIn, sizeof(struct sockaddr)) < 0){
        std::string errorMsg = "Erro com o bind do socket na porta " + std::to_string(port);
        throw std::runtime_error(errorMsg);
    } 
}

void Socket::createSocket(){
    if((this->socketFD = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
        std::string errorMsg = "Não foi possível obter o socket na porta " + std::to_string(port);
        throw std::runtime_error(errorMsg);
    }
}

int Socket::getSocketDescriptor(){
    return socketFD;
}

// Configura as informações do servidor
void Socket::setServerBindInfo(){
    serverAddrIn.sin_family = AF_INET;
    serverAddrIn.sin_port = htons(port);
    serverAddrIn.sin_addr.s_addr = debug ? htonl(INADDR_LOOPBACK) : INADDR_ANY; // Endereço utilizado para bind
    serverLen = sizeof(struct sockaddr_in);
}

// Atualiza as informações do último cliente que enviou um pacote	
void Socket::setLastClientInfo(struct sockaddr_in clientAddrIn){
    this->lastClientAddrIn = clientAddrIn;
    this->lastClientLen = sizeof(clientAddrIn);
}

// Define que o socket irá receber pacotes em broadcast. Utilizado pelos clientes
void Socket::setSocketBroadcastToTrue(){
    int yes = 1;
    setsockopt(socketFD, SOL_SOCKET, SO_BROADCAST, (char*)&yes, sizeof(yes));
}

void Socket::setSocketBroadcastToFalse(){
    int no = 0;
    setsockopt(socketFD, SOL_SOCKET, SO_BROADCAST, (char*)&no, sizeof(no));
}

struct in_addr Socket::getServerBinaryNetworkAddress(){
    return serverAddrIn.sin_addr;
}

// FUNÇÕES DE TROCAS DE PACOTES

// SERVIDOR

// Envia um pacote para um cliente
void Socket::sendPacketToClient(struct packet* sendPacketServer, struct sockaddr_in clientAddrIn){
    int n = sendto(socketFD, sendPacketServer, sizeof(struct packet), 0, (const struct sockaddr *) &clientAddrIn, sizeof(struct sockaddr_in));
    if (n < 0){
        std::string errorMsg = "Erro ao enviar um pacote para o servidor na porta " + std::to_string(port);
        throw std::runtime_error(errorMsg);
    }
}


// Recebe um pacote de um cliente e retorna as informações do cliente que enviou o pacote
struct sockaddr_in Socket::receivePacketFromClients(struct packet* recvPacketServer){
    sockaddr_in clientAddrIn;
    socklen_t clientLen = sizeof(clientAddrIn);
    int n = recvfrom(socketFD, recvPacketServer, sizeof(struct packet), 0, (struct sockaddr *) &clientAddrIn, &clientLen);
    if (n < 0){
        std::string errorMsg = "Erro ao receber um pacote do cliente na porta " + std::to_string(port);
        throw std::runtime_error(errorMsg);
    }
    return clientAddrIn;

} 

// CLIENTE

// Envia um pacote para o servidor, e especificando o tipo de envio
void Socket::sendPacketToServer(struct packet* sendPacketClient, int type, struct hostent* server){
    if (type == BROADCAST){
        serverAddrIn.sin_addr.s_addr = INADDR_BROADCAST;
    }
    else if (type == LOOPBACK){
        serverAddrIn.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    }
    else if (type == DIRECT_TO_SERVER){
        if (server == NULL){
            std::string errorMsg = "Erro: Servidor não definido na porta " + std::to_string(port);
            throw std::runtime_error(errorMsg);
        }
       serverAddrIn.sin_addr = *((struct in_addr *)server->h_addr);
    }
    
    int n = sendto(socketFD, sendPacketClient, sizeof(struct packet), 0, (const struct sockaddr *) &serverAddrIn, sizeof(struct sockaddr_in));
    if (n < 0){
        std::string errorMsg = "Erro ao enviar um pacote para o cliente na porta " + std::to_string(port);
        throw std::runtime_error(errorMsg);
    }
}

// Envia um pacote para o último cliente que enviou um pacote para o servidor
void Socket::sendPacketToLastSeenClient(struct packet* sendPacketServer){
    int n = sendto(socketFD, sendPacketServer, sizeof(struct packet), 0, (const struct sockaddr *) &lastClientAddrIn, sizeof(struct sockaddr_in));
    if (n < 0){
        std::string errorMsg = "Erro ao enviar um pacote para o último cliente na porta " + std::to_string(port);
        throw std::runtime_error(errorMsg);
    }
}

// Recebe um pacote do servidor
void Socket::receivePacketFromServer(struct packet* recvPacketClient){
    int n = recvfrom(socketFD, recvPacketClient, sizeof(struct packet), 0, (struct sockaddr *) &serverAddrIn, &serverLen);
    if (n < 0){
        std::string errorMsg = "Erro ao receber um pacote do servidor na porta " + std::to_string(port);
        throw std::runtime_error(errorMsg);
    }
}

