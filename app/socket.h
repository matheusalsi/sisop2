#ifndef SOCKET_H
#define SOCKET_H

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <strings.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <thread>
#include <linux/if.h>
#include <string.h>
#include "packet.h"

#define BROADCAST 1 
#define DIRECT_TO_SERVER 2
#define LOOPBACK 3


class Socket{
    private:

    struct sockaddr_in serverAddrIn;
    socklen_t serverLen;
    struct sockaddr_in lastClientAddrIn;
    socklen_t lastClientLen;

    int socketFD;
    int port;
    bool debug;

    void createSocket();
    // Configura as informações do servidor que serão utilizadas para fazer o bind
    void setServerInfo();
   
    public:

    Socket(int port, bool debug);

    // Cria um socket, e define as informações que serão utilizadas para bindar o servidor na porta 
    void openSocket();
    void closeSocket();
    // Binda o servidor na porta especificada
    void bindSocket();

    int getSocketDescriptor();
    struct in_addr getServerBinaryNetworkAddress();

    // Seta quanto tempo o socket vai ficar esperando receber um pacote (se não utilizar essa função ele vai esperar indefinidamente)
    void setSocketTimeoutMS(int time);
    // Define que o socket irá receber ou não pacotes em broadcast. Utilizado pelos clientes
    void setSocketBroadcastToTrue();
    void setSocketBroadcastToFalse();

    // Envia um pacote para um cliente
    void sendPacketToClient(struct packet* sendPacketServer, struct sockaddr_in clientAddrIn); 
    // Recebe um pacote de um cliente e retorna as informações do cliente que enviou o pacote
    bool receivePacketFromClients(struct packet* recvPacketServer, sockaddr_in& clientAddrIn);
    // Envia um pacote para o servidor, especificando o tipo de envio
    void sendPacketToServer(struct packet* sendPacketClient, int type,struct sockaddr_in* serverAddrIn);
    // Recebe um pacote do servidor 
    bool receivePacketFromServer(struct packet* recvPacketClient);
    
    
};

#endif 
