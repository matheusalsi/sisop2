#include "interface.h"


void InterfaceSS::stop(){
    handleExit();
    WOLSubsystem::stop();
}

void InterfaceSS::run(){
    hasTableUpdates = true;
    exiting = false;
    // std::thread printThread(&InterfaceSS::printInterfaceThread, this);
    std::thread inputThread(&InterfaceSS::inputThread, this);

    while(isRunning()){

        
        if(mailBox.isEmpty("M_OUT -> I_IN")){
            #ifdef DEBUG
            std::clog << "INTERFACE: ";
            std::clog << "Requisitando atualização da tabela" << std::endl;
            #endif

            std::string msg("REQUEST_UPDATE");
            mailBox.writeMessage("M_IN <- I_OUT", msg);
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        else{
            std::string msg;
            mailBox.readMessage("M_OUT -> I_IN", msg);
            #ifdef DEBUG
            std::clog << "INTERFACE: ";
            std::clog << "Recebi mensagem de GERENCIAMENTO: " << msg << std::endl;
            #endif
            std::vector<std::string> messageParameters;
            std::string messageFunction;
            getFunctionAndParametersFromMessage(msg, messageFunction, messageParameters);
            if(messageFunction == "SEND_UPDATE") {
                
            } else if(messageFunction == "SEND_MAC"){
                //todo
            }
            
            
            handleUpdateMessage(msg);
        }

    }
    // printThread.join();
    inputThread.join();

}

void InterfaceSS::handleUpdateMessage(std::string msg){
    #ifdef DEBUG
    std::clog << "INTERFACE: ";
    std::clog << "Mensagem de GERENCIAMENTO: " << msg << std::endl;
    #endif

    tableLock.lock();
    localTable.updateLineFromMessage(msg);
    hasTableUpdates = true;
    tableLock.unlock();

}

void InterfaceSS::printInterfaceThread(){

    while(isRunning()){
        if(!hasTableUpdates){
            continue;
        }
        tableLock.lock();

        // Há atualizações, logo printamos
        std::system("clear"); // Limpa tela (linux)

        if(exiting){
            std::cout << "Notificando manager e encerrando..." << std::endl;
            hasTableUpdates = false;
        }
        else{
            auto end = std::chrono::system_clock::now();
            std::time_t end_time = std::chrono::system_clock::to_time_t(end);
            if(isManager()){
                std::cout << "ESTAÇÃO MANAGER" << std::endl;
            }
            else{
                std::cout << "ESTAÇÃO PARTICIPANTE" << std::endl;
            }
            std::cout << "Última atividade: " << std::ctime(&end_time);

            localTable.printToConsole(); // Exibe tabela
        } 
        
        hasTableUpdates = false;
        tableLock.unlock();
        
    }
    
}

void InterfaceSS::inputThread(){
    std::string input;
    std::regex regex("wakeup (.*)");
    std::smatch match;
    while(isRunning()){
        std::getline(std::cin, input); // Pega espaços
        if (isManager()){ // Fazer aqui o WAKEUP hostname
            if(std::regex_search(input, match, regex)){
                
                auto hostname = match[0].str();
                auto mac = localTable.getMacFromHostname(hostname);
                std::string wakeonlan = "wakeonlan ";
                wakeonlan.append(mac);
                
                std::cout << "Enviando comando \"" << wakeonlan << "\"" << std::endl;

                system(wakeonlan.c_str());

            }
        }
        else if (input == "EXIT"){
            handleExit();
        }
    }
    
}

void InterfaceSS::handleExit(){
    if(exiting){
        return;
    }


    std::string input;
    
    exiting = true;
    
    if (!isManager()){
        hasTableUpdates = true; // Notifica thread the print para exibir mensagem
        mailBox.writeMessage("D_IN <- I_OUT", input); // Participante avisa para o seu discovery que vai sair
        while(mailBox.isEmpty("D_OUT -> I_IN")){ // Manager espera o seu discovery avisar que o participante foi removido da tabela
            exiting = true; 
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        #ifdef DEBUG
        std::clog << "Já fui removido da tabela. Encerrando o programa..." << std::endl;
        #endif
    }
    
    // Força parada do programa
    setRunning(false);

}

void InterfaceSS::getFunctionAndParametersFromMessage(std::string message, std::string &function, std::vector<std::string> &parameters){
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