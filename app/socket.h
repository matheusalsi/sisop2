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

class Socket{
    private:

    int socketFD;
    int port;
   
    public:

    Socket(int port);
    // Configurações do socket
    void openSocket();
    void bindSocket();
    void closeSocket();
    // Seta quanto tempo o socket vai ficar esperando receber um pacote (se não utilizar essa função ele vai esperar indefinidamente)
    void setSocketTimeoutMS(int time);
    // Define que o socket irá receber ou não pacotes em broadcast. Utilizado pelos clientes
    void setSocketBroadcastToTrue();
    void setSocketBroadcastToFalse();
    /* 
    * Envia um pacote para um endereço
    * @param packet: Pacote a ser enviado
    * @param send_type: tipo de envio (BROADCAST ou DIRECT_TO_IP)
    * @param portDst: porta de destino do pacote
    * @param ipDstStr: ip de destino do pacote (se send_type == DIRECT_TO_IP)
    * @return: true se o pacote foi enviado com sucesso, false caso contrário
    */
    bool sendPacket(struct packet& packet, int send_type, uint16_t portDst, std::string* ipDstStr=NULL);
    /* 
    * Recebe um pacote de um endereço
    * @param packet: referência que será atualizada com o pacote recebido
    * @param portSrc: refêrencia que será atualizada com a porta de origem do pacote
    * @param ipSrcStr: refêrencia que será atualizada com o ip de origem do pacote  
    * @return: true se o pacote foi recebido com sucesso, false caso contrário
    */
    bool receivePacket(struct packet& packet, uint16_t& portSrc, std::string& ipSrcStr); 
};

#endif 
