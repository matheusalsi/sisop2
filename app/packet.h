#ifndef PACKET_H
#define PACKET_H
// Bits: 
// 0 - Acknowledge 
// 2-1 - Operação
// 4-3 - Tipo
#define ACKNOWLEDGE                     0b00001

// Ops
#define SLEEP_SERVICE_DISCOVERY_FIND    0b00000
#define SLEEP_SERVICE_DISCOVERY_EXIT    0b00010

#define SLEEP_STATUS_REQUEST_CORRECTION 0b00010

#define BACKUP_INSERT                   0b00000
#define BACKUP_REMOVE                   0b00010
#define BACKUP_UPDATE                   0b00100

// Tipos
#define SLEEP_SERVICE_DISCOVERY         0b01000
#define SLEEP_STATUS_REQUEST            0b10000
#define BACKUP_MESSAGE                  0b11000



typedef struct packet{
    unsigned short int type; //Tipo do pacote (p.ex. DATA | CMD)
    char _payload[64]; //Dados da mensagem (será utilizado para pegar o Hostname e MAC)

} packet;

#endif 