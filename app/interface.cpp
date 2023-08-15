#include "interface.h"

// void InterfaceSS::userInputInterface(){
    
//     std::string input;
    
//     std::cout << "Digite o comando: " << std::endl;
//     std::cin >> input;

//     if (input == "" && g_isManager){ // Fazer aqui o WAKEUP hostname
//         /* wakeonlan é chamado com: 
//         std::string wakeonlan = "WAKEONLAN" + mac
//         system(wakeonlan.c_str())
//         */
//        ;
//     }
//     else if (input == "EXIT"){
//         if (!g_isManager){
//             mailBox.writeMessage("D_IN <- I_OUT", input); // Participante avisa para o seu discovery que vai sair
//             while(mailBox.isEmpty("D_OUT -> I_IN")){ // Manager espera o seu discovery avisar que o participante foi removido da tabela
//                 std::cout << "Esperando confirmação de saída do meu discovery" << std::endl;; // Espera chegar a confirmação 
//                 std::this_thread::sleep_for(std::chrono::milliseconds(500));
//             }
//             #ifdef DEBUG
//             std::cout << "Já fui removido da tabela. Encerrando o programa..." << std::endl;
//             #endif
//         }
//         setRunning(false);
//     }
    
// }

void InterfaceSS::start(){
    WOLSubsystem::start();
    waitingInput = false;
}

void InterfaceSS::stop(){
    WOLSubsystem::stop();
}


void InterfaceSS::run(){

    std::thread printThread(&InterfaceSS::printInterfaceThread, this);
    std::thread inputThread(&InterfaceSS::inputThread, this);

    while(isRunning()){

    }

    if(waitingInput){
        // Força terminação da thread de input
        pthread_cancel(inputThread.native_handle());
        printLock.unlock();
    }
    inputThread.join();
    printThread.join();

}

// Reticências (efeito visual)
void printWaitingDots(){
    static uint8_t n_dots = 0;
    static std::chrono::system_clock::time_point lastUpdate = std::chrono::system_clock::now();

    switch(n_dots){
        case 1: std::cout << "."; break; 
        case 2: std::cout << ".."; break; 
        case 3: std::cout << "..."; break; 
        case 4: n_dots = 0;
    }

    if(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - lastUpdate).count() <= 1000){
        return;
    }
    lastUpdate = std::chrono::system_clock::now();

    n_dots++;
}

void InterfaceSS::printInterfaceThread(){

    while(isRunning()){
        // Controle do console
        printLock.lock();

        // Caso especial do "EXIT"
        if(g_exiting){
            return;
        }

        #ifndef DEBUG
        std::system("clear"); // Limpa tela (linux)
        #endif

        // Caso especial de eleições
        if(g_electionHappening){
            std::cout << "Sem respostas do manager (" << tableManager->getManagerIP() << ")" << std::endl;
            std::cout << "Eleição em progresso";
            printWaitingDots();
            std::cout << std::endl << electionLogger.getLogsString();
        }
        else{

            auto end = std::chrono::system_clock::now();
            std::time_t end_time = std::chrono::system_clock::to_time_t(end);
            if(g_isManager){
                std::cout << "ESTAÇÃO MANAGER" << std::endl;
            }
            else{
                std::cout << "ESTAÇÃO PARTICIPANTE" << std::endl;
            }
            std::cout << "Última atividade: " << std::ctime(&end_time);

            // Mostra quem é o manager
            if(!g_isManager){
                if(g_foundManager){
                    std::cout << "IP do manager atual: " << tableManager->getManagerIP() << std::endl; 
                }
                else{
                    std::cout << "Procurando manager";
                    printWaitingDots();
                    std::cout << std::endl;
                }
            }

            // Obtém tabela
            auto tableString = tableManager->getTablePrintString();

            std::cout << tableString;

            // Informações de como realizar input
            std::cout << "Aperte ENTER para entrar no modo de input" << std::endl;

            // Log de mensagens
            std::cout << "LOGS:" << std::endl;
            std::cout << logger.getLogsString();

        }

        // Fim de printing na tela
        printLock.unlock();  

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

}

void InterfaceSS::inputThread(){
    std::string input;
    std::regex regex("wakeup (.*)");
    std::smatch match;

    bool validInput;
    
    while(isRunning()){
        
        // Caso especial do "EXIT". Previne que cin "tranque"
        if(g_exiting){
            return;
        }

        // Sinaliza que está esperando input
        waitingInput = true;

        // Espera por ENTER para entrar em modo de input
        input = std::cin.get();

        if(input != "\n"){
            waitingInput = false;
            continue;
        }
        
        // Enter pressionado; força parada de print
        printLock.lock();

        std::cout << "(MODO INPUT)" << std::endl;
        std::cout << "Digite seu input:" << std::endl;

        std::getline(std::cin, input); // Pega espaços

        waitingInput = false;
        
        if (g_isManager){ // Fazer aqui o WAKEUP hostname
            if(std::regex_search(input, match, regex)){
                
                validInput = true;

                // WakeOnLan
                
                auto hostname = match[1].str();

                auto mac = tableManager->getMacFromHostname(hostname);
                if(mac.empty()){
                    std::cout << "Não foi possível obter o mac desse hostname!" << std::endl;
                }
                else{
                    std::string wakeonlan = "wakeonlan ";
                    wakeonlan.append(mac);
                    
                    std::cout << "Enviando comando \"" << wakeonlan << "\"" << std::endl;

                    system(wakeonlan.c_str());
                }
                

            }
        }
        else if (input == "EXIT"){
            validInput = true;
            std::cout << "Encerrando o programa..." << std::endl;
            g_exiting = true;
            printLock.unlock();
            return;

        }

        // Input inválido
        if(!validInput){
            std::cout << "Input inválido" << std::endl;
        }
        // Reset
        validInput = false;

        std::cout << "Voltando à tela das tabelas em 5 segundos" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));

        printLock.unlock();

    }
    
}

// void InterfaceSS::handleExit(){
//     if(exiting){
//         return;
//     }
    
//     std::cout << "ABLUBLUEEEE\n";

//     std::string input;
    
//     exiting = true;
    
//     if (!g_isManager){
//         hasTableUpdates = true; // Notifica thread the print para exibir mensagem
//         mailBox.writeMessage("D_IN <- I_OUT", input); // Participante avisa para o seu discovery que vai sair
//         while(mailBox.isEmpty("D_OUT -> I_IN")){ // Manager espera o seu discovery avisar que o participante foi removido da tabela
//             exiting = true; 
//             std::this_thread::sleep_for(std::chrono::milliseconds(500));
//         }
//         #ifdef DEBUG
//         std::clog << "Já fui removido da tabela. Encerrando o programa..." << std::endl;
//         #endif
//     }
    
//     // Força parada do programa
//     g_exiting = true;
//     setRunning(false);

// }
