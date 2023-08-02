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
#define DIRECT_TO_IP 2
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

    void setAddrInInfo(struct sockaddr_in& sockAddrIn, struct in_addr addrIn);
   
    public:

    Socket(int port);

    void createSocket();
    void setServerInfo();

    // Abre um socket, obtendo seu descritor
    void oldOpenSocket();
    // Fecha o socket
    void closeSocket();
    // Binda o servidor na porta especificada
    void oldBindSocket();
    int getSocketDescriptor();
    struct in_addr getServerBinaryNetworkAddress();


    void openSocket();
    void bindSocket();

    // Seta quanto tempo o socket vai ficar esperando receber um pacote (se não utilizar essa função ele vai esperar indefinidamente)
    void setSocketTimeoutMS(int time);
    // Define que o socket irá receber ou não pacotes em broadcast. Utilizado pelos clientes
    void setSocketBroadcastToTrue();
    void setSocketBroadcastToFalse();

    // Envia um pacote, especificando o tipo de envio e retornando se teve sucesso no envio ou não
    int sendPacket(struct packet& packet, int send_type, char ipDst[INET_ADDRSTRLEN]);
    // Recebe um pacote e atualiza as informações de que enviou o pacote, retornando se teve sucesso no recebimento ou não
    int receivePacket(struct packet& packet, char ipSrc[INET_ADDRSTRLEN]);

    // Envia um pacote para um cliente
    void oldsendPacketToClient(struct packet* sendPacketServer, struct sockaddr_in clientAddrIn); 
    // Recebe um pacote de um cliente e retorna as informações do cliente que enviou o pacote
    bool oldreceivePacketFromClients(struct packet* recvPacketServer, sockaddr_in& clientAddrIn);
    // Envia um pacote para o servidor, especificando o tipo de envio
    void oldsendPacketToServer(struct packet* sendPacketClient, int type,struct sockaddr_in* serverAddrIn);
    // Recebe um pacote do servidor 
    bool oldreceivePacketFromServer(struct packet* recvPacketClient);
    
    
};

#endif 
