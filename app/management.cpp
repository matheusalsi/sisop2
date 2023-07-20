#include "management.h"

// void ManagementSS::start(){
//     WOLSubsystem::start();
// }

void ManagementSS::run(){
    // IPs recentemente adicionados e removidos
    std::vector<std::string> addedIPs;
    std::vector<std::string> removedIPs;


    while(isRunning()){
        addedIPs.clear();
        removedIPs.clear();

        /*
        
            COMUNICAÇÃO COM DISCOVERY: ATUALIZA QUEM ESTÁ NA TABELA
        
        */
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

                // Ip adicionado
                addedIPs.push_back(ip);

                if (!table.addLine(hostname, mac, ip, std::string("N/A")))
                    std::cout << "Não foi possível adicionar o: " << ip << " à tabela" << std::endl;
            }
            else if (messageFunction == "REMOVE_CLIENT"){
                std::string ip = messageParameters[0];

                // Ip removido
                removedIPs.push_back(ip);

                if (!table.removeLine(ip))
                    std::cout << "Não foi possível remover o: " << ip << std::endl;
            }

            
        }
        /*
        
            COMUNICAÇÃO COM MONITORAMENTO: DOS RELATOS DE MONITORAMENTO, PEGA
            OS QUAIS TEM IP NA TABELA ATUALMENTE. APÓS, ENVIA AS MODIFICAÇÕES
        
        */

       // IMPLEMENTAR COMO THREAD?
        while(isRunning() && !mailBox.isEmpty("MO_OUT -> M_IN")){
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

                // Apenas atualiza na tabela aqueles que permaneceram após sincronização com discovery
                if(table.hasIP(clientIP)){
                    table.updateLineStatus(clientIP, clientStatus);
                }
            }
        }

        // Atualiza monitoring sobre adições/remoções da tabela
        std::string msg;
        for(auto ip : addedIPs){
            msg = "+";
            msg.append(ip);
            mailBox.writeMessage("MO_IN <- M_OUT", msg);
        }
        for(auto ip : removedIPs){
            msg = "-";
            msg.append(ip);
            mailBox.writeMessage("MO_IN <- M_OUT", msg);
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

            // Interface requisitando a tabela
            if(messageFunction == "REQUEST_UPDATE"){
                std::string msg = "SEND_UPDATE";
                msg.append("&");
                msg.append(table.tableToString());
                mailBox.writeMessage("I_IN <- M_OUT", msg);
            }
            // table.printToConsole();

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


