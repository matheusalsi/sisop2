#ifndef PACKET_H
#define PACKET_H

#define ACKNOWLEDGE 0b0001
#define SLEEP_SERVICE_DISCOVERY 0b0100
#define SLEEP_SERVICE_DISCOVERY_FIND 0b0000
#define SLEEP_SERVICE_DISCOVERY_EXIT 0b0010

#define SLEEP_STATUS_REQUEST 0b1000
#define SLEEP_STATUS_REQUEST_

typedef struct packet{
    unsigned short int type; //Tipo do pacote (p.ex. DATA | CMD)
    char _payload[64]; //Dados da mensagem (será utilizado para pegar o Hostname e MAC)

} packet;

#endif 