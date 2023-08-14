#include "management.h"

// void TableManager::start(){
//      WOLSubsystem::start();

// }

// void TableManager::stop(){
//      WOLSubsystem::start();
// }

TableManager::TableManager(bool isManager) : backupSocket(MANAGEMENT_PORT) {
    this->isManager = isManager;
    
    backupSocket.openSocket();
    backupSocket.setSocketTimeoutMS(10);
    if(!isManager){
        // Nem sempre vai estar agindo como cliente, mas o bind não dá problema mesmo assim
        
        backupSocket.bindSocket();
        setBackupStatus(true);
    }
}

TableManager::~TableManager(){
    setBackupStatus(false);
    backupSocket.closeSocket();
}

void TableManager::setBackupStatus(bool isBackup){
    
    if(isBackup != runningBackup){
        runningBackup = isBackup; // Atualiza antes para garantir que thread sabe do estado atual
        if(isBackup){
            // Começa thread que espera por atualizações
            thrBackupListener = new std::thread(&TableManager::backupListenerThread, this);
        }
        else{
            // Finaliza thread de backup
            thrBackupListener->join();
            delete thrBackupListener;
        }
    }
    
}

void TableManager::backupListenerThread(){
    // Encerra thread caso não esteja em modo de backup
    packet recvPacket;
    std::string serverIpStr;
    uint16_t serverPort;
    
    std::string receivedMessage;
    IpInfo receivedInfo;
    while(runningBackup){
        
        if(!backupSocket.receivePacket(recvPacket, serverPort, serverIpStr)){
            continue;
        } 

        receivedMessage = recvPacket._payload;

        auto firstSep = receivedMessage.find('&');
        auto secondSep = receivedMessage.find('&', firstSep+1);
        auto thirdSep = receivedMessage.find('&', secondSep+1);

        std::string ip          = receivedMessage.substr(0, firstSep);
        receivedInfo.hostname   = receivedMessage.substr(firstSep+1, secondSep-firstSep-1);
        receivedInfo.mac        = receivedMessage.substr(secondSep+1, thirdSep-secondSep-1);
        receivedInfo.awake      = (receivedMessage.at(thirdSep+1) == 'Y'); // Y/N
        
        #ifdef LOG_BACKUP
        std::stringstream logMsg;
        #endif 
        
        if(recvPacket.type == (BACKUP_MESSAGE | BACKUP_INSERT)){
            #ifdef LOG_BACKUP
            logMsg << "BACKUP: Mensagem de inserção recebida";
            #endif
            insertClient(ip, receivedInfo, false);

        }
        else if(recvPacket.type == (BACKUP_MESSAGE | BACKUP_REMOVE)){
            #ifdef LOG_BACKUP
            logMsg << "BACKUP: Mensagem de remoção recebida";
            #endif
            removeClient(ip);
        }
        else if(recvPacket.type == (BACKUP_MESSAGE | BACKUP_UPDATE)){
            #ifdef LOG_BACKUP
            logMsg << "BACKUP: Mensagem de atualização recebida";
            #endif
            updateClient(receivedInfo.awake, ip);

        }
        else{
            continue;
        }

        #ifdef LOG_BACKUP
        logMsg << " (" << receivedInfo.hostname << " " << receivedInfo.mac << " " << receivedInfo.awake << ")";
        std::string msg = logMsg.str();
        logger.log(msg);
        #endif

        // Responde com acknowledge
        packet sendPacket;
        sendPacket.type = recvPacket.type | ACKNOWLEDGE;
        backupSocket.sendPacket(sendPacket, DIRECT_TO_IP, serverPort, &serverIpStr);

    }
}

void TableManager::sendBackupPacketToClients(uint8_t operation, std::string ip, IpInfo& ipInfo){
    packet sendPacket;
    packet receivePacket;
    uint16_t receivePort;
    std::string receiveIP;

    sendPacket.type = (BACKUP_MESSAGE | operation);
    
    std::string packetPayload;
    // Formato: IP&HOSTNAME&MAC&(Y/N)
    packetPayload.append(ip);
    packetPayload.append("&");
    packetPayload.append(ipInfo.hostname);
    packetPayload.append("&");
    packetPayload.append(ipInfo.mac);
    packetPayload.append("&");
    packetPayload.append(ipInfo.awake ? "Y" : "N");
    
    strcpy(sendPacket._payload, packetPayload.c_str()); 

    auto ips = knownIps; // Protegido pelo lock exterior
    int nTimeouts = 0;

    // Envia a todos os clientes
    for(auto ip : ips){
        while(nTimeouts < 10){
            backupSocket.sendPacket(sendPacket, DIRECT_TO_IP, MANAGEMENT_PORT, &ip);
            if(backupSocket.receivePacket(receivePacket, receivePort, receiveIP)){
                if(receivePacket.type == (BACKUP_MESSAGE | operation | ACKNOWLEDGE)){
                    break;
                }
            }
            nTimeouts++;
        }
        nTimeouts = 0;
    }
}


void TableManager::insertClient(std::string ip, IpInfo ipInfo, bool insertingManager){
    
    // Acesso à tabela
    tableLock.lock();

    // Adição à tabela (assume-se que não está na tabela ainda)
    ipStatusTable[ip] = ipInfo;
    knownIps.insert(ip);
    macHostnameMap[ipInfo.hostname] = ipInfo.mac;

    // Se o cliente adicionado é o gerenciador, atualiza o ip guardado
    if(insertingManager){
        managerIP = ip;
    }

    #ifdef DEBUG_TABLE
    std::clog << "Inserindo cliente de ip " << ip << std::endl;
    #endif

    // Atualiza clientes
    if(isManager){
        sendBackupPacketToClients(BACKUP_INSERT, ip, ipInfo);
        #ifdef LOG_BACKUP
        logger.log("BACKUP - Mensagem de inserçao enviada");
        #endif
    }

    // Libera acesso
    tableLock.unlock();

}

void TableManager::removeClient(std::string ip){
    // Acesso à tabela
    tableLock.lock();

    // Remoção da tabela (assume-se que já está na tabela)
    macHostnameMap.erase(ipStatusTable[ip].hostname);
    knownIps.erase(ip);
    ipStatusTable.erase(ip);

    // Se o cliente removido é o gerenciador, atualiza o ip guardado
    if(managerIP == ip){
        managerIP = "";
    }

    #ifdef DEBUG_TABLE
    std::clog << "Removendo cliente de ip " << ip << std::endl;
    #endif

    // Atualiza clientes
    if(isManager){
        IpInfo ipInfo;
        sendBackupPacketToClients(BACKUP_REMOVE, ip, ipInfo);
        #ifdef LOG_BACKUP
        logger.log("BACKUP - Mensagem de remoçao enviada");
        #endif
    }

    // Libera acesso
    tableLock.unlock();    
}

// Retorna lista de ips conhecidos
const std::set<std::string>* TableManager::getKnownIps(){
    // Acesso à tabela (lista de IPs)
    tableLock.lock();

    const auto r_ptr = &knownIps;

    // Libera acesso
    tableLock.unlock();

    return r_ptr;

}

// Atualização dos clientes. Retorna falso se cliente não existe mais na tabela (MONITORING)
bool TableManager::updateClient(bool awake, std::string ip){
    // Acesso à tabela
    tableLock.lock();

    // Checa se ip ainda está na tabela
    if(ipStatusTable.find(ip) == ipStatusTable.end()){
        // Libera acesso
        tableLock.unlock();
        return false;
    }

    // Atualiza status
    ipStatusTable[ip].awake = awake;
    #ifdef DEBUG_TABLE
    std::clog << "Atualizando status do cliente de ip " << ip << ": " << (awake ? "AWAKE" : "ASLEEP") << std::endl;
    #endif

    // Atualiza clientes
    if(isManager){
        IpInfo ipInfo;
        ipInfo.awake = awake;
        sendBackupPacketToClients(BACKUP_UPDATE, ip, ipInfo);
        #ifdef LOG_BACKUP
        logger.log("BACKUP - Mensagem de atualizaçao enviada");
        #endif
    }

    // Libera acesso
    tableLock.unlock();

    return true;


}

std::string TableManager::getTablePrintString(){

    std::map<std::string, IpInfo>::iterator it;
    std::stringstream ss;

    // Cabeçalho
    ss << std::setw(HOSTNAME_ROW_WIDTH) << "HOSTNAME";
    ss << " | ";
    ss << std::setw(MACADDRESS_ROW_WIDTH) << "ENDERECO MAC";
    ss << " | ";
    ss << std::setw(IPADDRESS_ROW_WIDTH) << "ENDERECO IP";
    ss << " | ";
    ss << std::setw(STATUS_ROW_WIDTH) << "STATUS";

    ss << std::endl;

    // Acesso à tabela
    tableLock.lock();
    // Linhas
    for(it = ipStatusTable.begin(); it != ipStatusTable.end(); it++){
        ss << std::setw(HOSTNAME_ROW_WIDTH) << it->second.hostname;
        ss << " | ";
        ss << std::setw(MACADDRESS_ROW_WIDTH) << it->second.mac;
        ss << " | ";
        ss << std::setw(IPADDRESS_ROW_WIDTH) << it->first;
        ss << " | ";
        ss << std::setw(STATUS_ROW_WIDTH);
        ss << (it->second.awake ? "awake" : "asleep");
        ss << std::endl;
    }
    // Fim de acesso
    tableLock.unlock();

    // Fim
    ss << std::setfill('-') << std::setw(HOSTNAME_ROW_WIDTH + MACADDRESS_ROW_WIDTH + IPADDRESS_ROW_WIDTH + STATUS_ROW_WIDTH + 9) << '-' << std::endl;
    ss << std::setfill(' ') << std::setw(1);


    return ss.str();
}

std::string TableManager::getMacFromHostname(std::string hostname){
    std::string r_str; // Retorna "" se mac não existe 
    // Acesso à tabela
    tableLock.lock();
    
    r_str = macHostnameMap[hostname];



    return r_str;

}

// Obtém IP do gerenciador
std::string TableManager::getManagerIP(){
    
    // Acesso à tabela
    tableLock.lock();

    auto r_str = managerIP;

    // Libera acesso
    tableLock.unlock();
    
    return r_str;
}

// void TableManager::run(){
//     // IPs recentemente adicionados e removidos
//     std::vector<std::string> addedIPs;
//     std::vector<std::string> removedIPs;


//     while(isRunning()){
//         addedIPs.clear();
//         removedIPs.clear();

//         /*
        
//             COMUNICAÇÃO COM DISCOVERY: ATUALIZA QUEM ESTÁ NA TABELA
        
//         */
//         if(!mailBox.isEmpty("D_OUT -> M_IN")){
//             std::string message;
//             mailBox.readMessage("D_OUT -> M_IN", message);
            
//             #ifdef DEBUG
//             std::clog << "GERENCIAMENTO: ";
//             std::clog << "Mensagem de DISCOVERY: " << message << std::endl; 
//             #endif

//             std::vector<std::string> messageParameters;
//             std::string messageFunction;
//             getFunctionAndParametersFromMessage(message, messageFunction, messageParameters);

//             if (messageFunction == "ADD_CLIENT" || messageFunction == "ADD_MANAGER"){
//                 std::string ip = messageParameters[0];
//                 std::string hostname = messageParameters[1];
//                 std::string mac = messageParameters[2];

//                 // Ip adicionado
//                 addedIPs.push_back(ip);

//                 if (!table.addLine(hostname, mac, ip, std::string("N/A")))
//                     std::cout << "Não foi possível adicionar o: " << ip << " à tabela" << std::endl;
//             }
//             else if (messageFunction == "REMOVE_CLIENT"){
//                 std::string ip = messageParameters[0];

//                 // Ip removido
//                 removedIPs.push_back(ip);

//                 if (!table.removeLine(ip))
//                     std::cout << "Não foi possível remover o: " << ip << std::endl;
//             }

            
//         }
//         /*
        
//             COMUNICAÇÃO COM MONITORAMENTO: DOS RELATOS DE MONITORAMENTO, PEGA
//             OS QUAIS TEM IP NA TABELA ATUALMENTE. APÓS, ENVIA AS MODIFICAÇÕES
        
//         */

//        // IMPLEMENTAR COMO THREAD?
//         while(isRunning() && !mailBox.isEmpty("MO_OUT -> M_IN")){
//             std::string message;
//             mailBox.readMessage("MO_OUT -> M_IN", message);
            
//             #ifdef DEBUG
//             std::clog << "GERENCIAMENTO: ";
//             std::clog << "Mensagem de MONITORING: " << message << std::endl; 
//             #endif

//             std::vector<std::string> messageParameters;
//             std::string messageFunction;
//             getFunctionAndParametersFromMessage(message, messageFunction, messageParameters);

//             if (messageFunction == "UPDATE_CLIENT_STATUS"){
//                 std::string clientIP = messageParameters[0];
//                 std::string clientStatus = messageParameters[1];

//                 // Apenas atualiza na tabela aqueles que permaneceram após sincronização com discovery
//                 if(table.hasIP(clientIP)){
//                     table.updateLineStatus(clientIP, clientStatus);
//                 }
//             }
//         }

//         // Atualiza monitoring sobre adições/remoções da tabela
//         std::string msg;
//         for(auto ip : addedIPs){
//             msg = "+";
//             msg.append(ip);
//             mailBox.writeMessage("MO_IN <- M_OUT", msg);
//         }
//         for(auto ip : removedIPs){
//             msg = "-";
//             msg.append(ip);
//             mailBox.writeMessage("MO_IN <- M_OUT", msg);
//         }


//         if(!mailBox.isEmpty("I_OUT -> M_IN")){
//             std::string message;
//             mailBox.readMessage("I_OUT -> M_IN", message);
//             #ifdef DEBUG
//             std::clog << "GERENCIAMENTO: ";
//             std::clog << "Mensagem de INTERFACE: " << message << std::endl; 
//             #endif
//             std::vector<std::string> messageParameters;
//             std::string messageFunction;
//             getFunctionAndParametersFromMessage(message, messageFunction, messageParameters);

//             // Interface requisitando a tabela
//             if(messageFunction == "REQUEST_UPDATE"){
//                 std::string msg;
//                 for(const auto& pair : table.getLines()){
//                     msg = "+";
//                     table.appendLineAsMessage(pair.first, msg);
//                     mailBox.writeMessage("I_IN <- M_OUT", msg);
//                 }
//             }
//             // table.printToConsole();

//         }

//     }
        
// }
