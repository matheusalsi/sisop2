#include "interface.h"

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



void InterfaceSS::userInputInterface(){
    
    std::string input;
    
    std::cout << "Digite o comando: " << std::endl;
    std::cin >> input;

    if (input == "WAKEUP" && isManager())
        ; // parsear para WAKEONLAN
    else if (input == "EXIT"){
        if (!isManager()){
            mailBox.writeMessage("D_IN", input); // Participante avisa para o seu discovery que vai sair
            while(mailBox.isEmpty("D3_OUT")){ // Manager espera o seu discovery avisar que o participante foi removido da tabela
                std::cout << "Esperando confirmação de saída do meu discovery" << std::endl;; // Espera chegar a confirmação 
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
            #ifdef DEBUG
            std::cout << "Já fui removido da tabela. Encerrando o programa..." << std::endl;
            #endif
        }
        setRunning(false);
    }
    
}

void InterfaceSS::run(){
    // bool b = true;

    while(isRunning()){
        
        userInputInterface();
        // printInterface();

        // // Exemplo
        // table.addLine("1.1.1.1", "01:01:01:01:01:01", "teste");
        // table.updateLineStatus("1.1.1.1", b);
        // b = !b;
        
        // std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

void InterfaceSS::printInterface(){
    std::system("clear"); // Limpa tela (linux)
    table.printToConsole(); // Exibe tabela
}