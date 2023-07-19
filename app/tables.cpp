#include "tables.h"

void WOLTableLine::appendToMessageString(std::string& msg){
    msg.append(ip);
    msg.append("&");
    msg.append(hostname);
    msg.append("&");
    msg.append(mac);
    msg.append("&");
    msg.append(status);
    msg.append("&");
}


void WOLTable::printToConsole(){
    //https://stackoverflow.com/questions/26281979/c-loop-through-map
    std::map<std::string, WOLTableLine>::iterator it;

    // Cabeçalho
    std::cout << std::setw(IPADDRESS_ROW_WIDTH) << "ENDERECO IP";
    std::cout << " | ";
    std::cout << std::setw(hostname_max_len) << "HOSTNAME";
    std::cout << " | ";
    std::cout << std::setw(MACADDRESS_ROW_WIDTH) << "ENDERECO MAC";
    std::cout << " | ";
    std::cout << std::setw(STATUS_ROW_WIDTH) << "STATUS";

    std::cout << std::endl;

    // Linhas
    for(it = lines.begin(); it != lines.end(); it++){
        std::cout << std::setw(IPADDRESS_ROW_WIDTH) << it->second.ip;
        std::cout << " | ";
        std::cout << std::setw(hostname_max_len) << it->second.hostname;
        std::cout << " | ";
        std::cout << std::setw(MACADDRESS_ROW_WIDTH) << it->second.mac;
        std::cout << " | ";
        std::cout << std::setw(STATUS_ROW_WIDTH) << it->second.status;
        std::cout << std::endl;
    }

    // Fim
    std::cout << std::setfill('-') << std::setw(hostname_max_len + MACADDRESS_ROW_WIDTH + IPADDRESS_ROW_WIDTH + STATUS_ROW_WIDTH + 9) << '-' << std::endl;
    std::cout << std::setfill(' ') << std::setw(1);

}

bool WOLTable::addLine(std::string hostname, std::string macaddr, std::string ipaddr){
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
    if(lines.find(ipaddr) == lines.end()){
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

bool WOLTable::checkLineStatusDiff(std::string ipaddr, std::string status){
    return lines[ipaddr].status.compare(status);
}

void WOLTable::updateLineStatus(std::string ipaddr, std::string status){
    lines[ipaddr].status = status;
}

void WOLTable::appendLineAsMessage(std::string ipaddr, std::string& msg){
    lines[ipaddr].appendToMessageString(msg);
}

void WOLTable::updateLineFromMessage(std::string& msg){
    std::string body = msg.substr(msg.find('&')+1); // Remove cabeçalho
    std::stringstream ss(body);
    std::string ipaddr; 
    std::getline(ss, ipaddr, '&'); // IP
    lines[ipaddr].ip = ipaddr;
    std::getline(ss, lines[ipaddr].hostname, '&'); // HOSTNAME
    std::getline(ss, lines[ipaddr].mac, '&'); // MAC
    std::getline(ss, lines[ipaddr].status, '&'); // STATUS
}