#include "management.h"

// void ManagementSS::start(){
//     WOLSubsystem::start();
// }

void ManagementSS::run(){
    while(isRunning()){
        while(!mailBox.isEmpty("D_OUT -> M_IN")){
            std::string message;
            mailBox.readMessage("D_OUT -> M_IN", message);
            std::cout << "Mensagem de DISCOVERY: " << message << std::endl; 

            std::vector<std::string> messageParameters;
            std::string messageFunction;
            getFunctionAndParametersFromMessage(message, messageFunction, messageParameters);

            if (messageFunction == "ADD_CLIENT" || messageFunction == "ADD_MANAGER"){
                std::string ip = messageParameters[0];
                std::string hostname = messageParameters[1];
                std::string mac = messageParameters[2];

                if (!table.addLine(hostname, mac, ip))
                    std::cout << "Não foi possível adicionar o: " << ip << " à tabela" << std::endl;
            }
            else if (messageFunction == "REMOVE_CLIENT"){
                std::string clientIP = messageParameters[0];

                if (!table.removeLine(clientIP))
                    std::cout << "Não foi possível remover o: " << clientIP << std::endl;
            }
            table.printToConsole();
            
        }

        while(!mailBox.isEmpty("MO_OUT -> M_IN")){
            std::string message;
            mailBox.readMessage("MO_OUT -> M_IN", message);
            std::cout << "Mensagem de MONITORING: " << message << std::endl; 
            std::vector<std::string> messageParameters;
            std::string messageFunction;
            getFunctionAndParametersFromMessage(message, messageFunction, messageParameters);

            if (messageFunction == "UPDATE_CLIENT_STATUS"){
                std::string clientIP = messageParameters[0];
                std::string clientStatus = messageParameters[1];

                table.updateLineStatus(clientIP, clientStatus);
            }
            table.printToConsole();
        }

        while(!mailBox.isEmpty("I_OUT -> M_IN")){
            std::string message;
            mailBox.readMessage("I_OUT -> M_IN", message);
            std::cout << "Mensagem de MONITORING: " << message << std::endl; 
            std::vector<std::string> messageParameters;
            std::string messageFunction;
            getFunctionAndParametersFromMessage(message, messageFunction, messageParameters);
        }
        
    }
}


void ManagementSS::getFunctionAndParametersFromMessage(std::string message, std::string &function, std::vector<std::string> &parameters){
    std::string delimiter = "&";
    std::string token;
    // Pega qual o tipo da função que será chamada
    int firstDelimiter = message.find(delimiter);
    function = message.substr(0, firstDelimiter);
    message.erase(0, firstDelimiter + 1);

    size_t start = 0;
    size_t end = message.find('&');
    while (end != std::string::npos) {
        std::string ip = message.substr(start, end - start);
        parameters.push_back(ip);
        start = end + 1;
        end = message.find('&',start);
    }
    std::string lastParameter = message.substr(start);
    parameters.push_back(lastParameter);
}

// void ManagementSS::stop(){
//     
// }

// void ManagementSS::processMessage(std::string message){

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

void WOLTable::updateLineStatus(std::string ipaddr, std::string status){
    lines[ipaddr].status = status;
}


