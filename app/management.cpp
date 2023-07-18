#include "management.h"

void ManagementSS::start(){
    WOLSubsystem::start();
}

void ManagementSS::run(){
    while(isRunning()){
        while(!mailBox.isEmpty("D_OUT -> M_IN")){
            std::string msg;
            mailBox.readMessage("D_OUT -> M_IN", msg);
            std::cout << "Mensagem de DISCOVERY: " << msg << std::endl; 
        }
        while(!mailBox.isEmpty("MO_OUT -> M_IN")){
            std::string msg;
            mailBox.readMessage("MO_OUT -> M_IN", msg);
            std::cout << "Mensagem de MONITORING: " << msg << std::endl; 
        }
    }
}

// void ManagementSS::stop(){
//     
// }

void WOLTable::printToConsole(){
    //https://stackoverflow.com/questions/26281979/c-loop-through-map
    std::map<std::string, WOLTableLine>::iterator it;

    // Cabeçalho
    std::cout << std::setw(hostname_max_len) << "HOSTNAME";
    std::cout << " | ";
    std::cout << std::setw(MACADDRESS_ROW_WIDTH) << "ENDERECO MAC";
    std::cout << " | ";
    std::cout << std::setw(IPADDRESS_ROW_WIDTH) << "ENDERECO IP";
    std::cout << " | ";
    std::cout << std::setw(STATUS_ROW_WIDTH) << "STATUS";

    std::cout << std::endl;

    // Linhas
    for(it = lines.begin(); it != lines.end(); it++){
        std::cout << std::setw(hostname_max_len) << it->second.hostname;
        std::cout << " | ";
        std::cout << std::setw(MACADDRESS_ROW_WIDTH) << it->second.mac;
        std::cout << " | ";
        std::cout << std::setw(IPADDRESS_ROW_WIDTH) << it->second.ip;
        std::cout << " | ";
        std::cout << std::setw(STATUS_ROW_WIDTH) << it->second.status;
        std::cout << std::endl;
    }

    // Fim
    std::cout << std::setfill('-') << std::setw(hostname_max_len + MACADDRESS_ROW_WIDTH + IPADDRESS_ROW_WIDTH + STATUS_ROW_WIDTH + 9) << '-' << std::endl;
    std::cout << std::setfill(' ') << std::setw(1);

}

bool WOLTable::addLine(std::string ipaddr, std::string macaddr, std::string hostname){
    if(lines.find(ipaddr) != lines.end()){
        return false;
    }

    // Atualiza tamanho da coluna hostname
    int hostname_len = hostname.length();
    if(hostname_len >  hostname_max_len){
        hostname_max_len = hostname_len;
    }

    lines[ipaddr] = WOLTableLine(hostname, macaddr, ipaddr, "N/A");
    return true;
}

bool WOLTable::removeLine(std::string ipaddr){
    if(lines.find(ipaddr) != lines.end()){
        return false;
    }

    lines.erase(ipaddr);

    // Atualiza tamanho do hostname. Por default, a chave maior está no fim.
    if(lines.empty()){
        hostname_max_len = HOSTNAME_ROW_WIDTH;
    }
    else{
        hostname_max_len = lines.rbegin()->second.hostname.length();
    }

    return true;
}

void WOLTable::updateLineStatus(std::string ipaddr, bool awake){
    lines[ipaddr].status = awake ? "AWAKE" : "ASLEEP";
}


