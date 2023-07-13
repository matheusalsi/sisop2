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
#include <linux/if.h>
#include <string.h>
#include "packet.h"

#define BROADCAST 1
#define TO_SERVER 2
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
    void setServerBindInfo();
    void setLastClientInfo(struct sockaddr_in);
    void setServerAddressInfo(int inadrr);

    public:

    Socket(int port, bool debug);

    void openSocket();
    void closeSocket();
    void bindSocket();
    void setSocketBroadcastToTrue();
    void setSocketBroadcastToFalse();
    int getSocketDescriptor();

    void sendPacketToClient(struct packet* sendPacketServer, struct sockaddr_in clientAddrIn);
    void sendPacketToLastSeenClient(struct packet* sendPacketServer);
    void sendPacketToServer(struct packet* sendPacketClient, int type, struct hostent* serverAddrIn);
    struct sockaddr_in receivePacketFromClients(struct packet* recvPacketServer);
    void receivePacketFromServer(struct packet* recvPacketClient);
    struct in_addr getServerBinaryNetworkAddress();
};

#endif 
