#include "management.h"

// void ManagementSS::start(){
//      WOLSubsystem::start();

// }

// void ManagementSS::stop(){
//      WOLSubsystem::start();
// }

void TableManager::insertClient(std::string ip, IpInfo ipInfo){
    
    // Acesso à tabela
    tableLock.lock();

    // Adição à tabela (assume-se que não está na tabela ainda)
    ipStatusTable[ip] = ipInfo;
    knownIps.insert(ip);

    #ifdef DEBUG_TABLE
    std::clog << "Inserindo cliente de ip " << ip << std::endl;
    #endif


    // Libera acesso
    tableLock.unlock();

}

void TableManager::removeClient(std::string ip){
    // Acesso à tabela
    tableLock.lock();

    // Remoção da tabela (assume-se que já está na tabela)
    knownIps.erase(ip);

    #ifdef DEBUG_TABLE
    std::clog << "Removendo cliente de ip " << ip << std::endl;
    #endif


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
    std::cout << std::setfill('-') << std::setw(HOSTNAME_ROW_WIDTH + MACADDRESS_ROW_WIDTH + IPADDRESS_ROW_WIDTH + STATUS_ROW_WIDTH + 9) << '-' << std::endl;
    std::cout << std::setfill(' ') << std::setw(1);


    return ss.str();
}


// void ManagementSS::run(){
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
