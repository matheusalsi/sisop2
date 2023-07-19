#include "management.h"

// void ManagementSS::start(){
//     WOLSubsystem::start();
// }

void ManagementSS::run(){
    while(isRunning()){
        if(!mailBox.isEmpty("D_OUT -> M_IN")){
            std::string message;
            mailBox.readMessage("D_OUT -> M_IN", message);
            
            #ifdef DEBUG
            std::clog << "GERENCIAMENTO: ";
            std::clog << "Mensagem de DISCOVERY: " << message << std::endl; 
            #endif

            std::vector<std::string> messageParameters;
            std::string messageFunction;
            getFunctionAndParametersFromMessage(message, messageFunction, messageParameters);

            if (messageFunction == "ADD_CLIENT" || messageFunction == "ADD_MANAGER"){
                std::string ip = messageParameters[0];
                std::string hostname = messageParameters[1];
                std::string mac = messageParameters[2];

                // Adiciona IP aos modificados
                recentlyUpdatedIPs.insert(ip);

                if (!table.addLine(hostname, mac, ip))
                    std::cout << "Não foi possível adicionar o: " << ip << " à tabela" << std::endl;
            }
            else if (messageFunction == "REMOVE_CLIENT"){
                std::string clientIP = messageParameters[0];

                // Adiciona IP aos modificados
                recentlyUpdatedIPs.insert(clientIP);

                if (!table.removeLine(clientIP))
                    std::cout << "Não foi possível remover o: " << clientIP << std::endl;
            }

            
        }

        if(!mailBox.isEmpty("MO_OUT -> M_IN")){
            std::string message;
            mailBox.readMessage("MO_OUT -> M_IN", message);
            
            #ifdef DEBUG
            std::clog << "GERENCIAMENTO: ";
            std::clog << "Mensagem de MONITORING: " << message << std::endl; 
            #endif

            std::vector<std::string> messageParameters;
            std::string messageFunction;
            getFunctionAndParametersFromMessage(message, messageFunction, messageParameters);

            if (messageFunction == "UPDATE_CLIENT_STATUS"){
                std::string clientIP = messageParameters[0];
                std::string clientStatus = messageParameters[1];
                if(table.checkLineStatusDiff(clientIP, clientStatus)){
                    table.updateLineStatus(clientIP, clientStatus);
                    // Atualiza set de ips com atualizações não informadas à interface
                    recentlyUpdatedIPs.insert(clientIP);
                }
            }

        }

        if(!mailBox.isEmpty("I_OUT -> M_IN")){
            std::string message;
            mailBox.readMessage("I_OUT -> M_IN", message);
            #ifdef DEBUG
            std::clog << "GERENCIAMENTO: ";
            std::clog << "Mensagem de INTERFACE: " << message << std::endl; 
            #endif
            std::vector<std::string> messageParameters;
            std::string messageFunction;
            getFunctionAndParametersFromMessage(message, messageFunction, messageParameters);

            // Interface requisitando info sobre estado da tabela (houve atualizações?)
            if(messageFunction == "REQUEST_UPDATE"){
                if(recentlyUpdatedIPs.size() > 0){
                    std::string msg;
                    for(auto ip: recentlyUpdatedIPs){
                        // Checa se IP foi removido
                        if(table.hasIP(ip)){
                            msg = "TABLE_UPDATE&";
                        }
                        else{
                            msg = "TABLE_REMOVE&";
                        }


                        table.appendLineAsMessage(ip, msg);
                    }

                    // IPs não são mais considerados como "recentemente atualizados"
                    recentlyUpdatedIPs.clear();
                    // Envia resposta
                    mailBox.writeMessage("I_IN <- M_OUT", msg);
                }

            }

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


