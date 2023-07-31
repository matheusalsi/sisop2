#include "interface.h"

// void InterfaceSS::userInputInterface(){
    
//     std::string input;
    
//     std::cout << "Digite o comando: " << std::endl;
//     std::cin >> input;

//     if (input == "" && isManager()){ // Fazer aqui o WAKEUP hostname
//         /* wakeonlan é chamado com: 
//         std::string wakeonlan = "WAKEONLAN" + mac
//         system(wakeonlan.c_str())
//         */
//        ;
//     }
//     else if (input == "EXIT"){
//         if (!isManager()){
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

void InterfaceSS::stop(){
    //handleExit();
    WOLSubsystem::stop();
}

void InterfaceSS::run(){

    std::thread inputThread(&InterfaceSS::inputThread, this);

    while(isRunning()){
        printInterface();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    //inputThread.join();

}

void InterfaceSS::printInterface(){
    // Controle do console
    printLock.lock();

    std::system("clear"); // Limpa tela (linux)

    auto end = std::chrono::system_clock::now();
    std::time_t end_time = std::chrono::system_clock::to_time_t(end);
    if(isManager()){
        std::cout << "ESTAÇÃO MANAGER" << std::endl;
    }
    else{
        std::cout << "ESTAÇÃO PARTICIPANTE" << std::endl;
    }
    std::cout << "Última atividade: " << std::ctime(&end_time);

    // Obtém tabela
    auto tableString = tableManager->getTablePrintString();

    std::cout << tableString;

    // Informações de como realizar input
    std::cout << "Aperte ENTER para entrar no modo de input" << std::endl;

    // Fim de printing na tela
    printLock.unlock();

}

void InterfaceSS::inputThread(){
    std::string input;
    std::regex regex("wakeup (.*)");
    std::smatch match;

    bool validInput;
    
    while(isRunning()){
        
        // Espera por ENTER para entrar em modo de input
        input = std::cin.get();

        if(input != "\n"){
            continue;
        }
        
        // Enter pressionado; força parada de print
        printLock.lock();

        std::cout << "(MODO INPUT)" << std::endl;
        std::cout << "Digite seu input:" << std::endl;

        std::getline(std::cin, input); // Pega espaços
        
        if (isManager()){ // Fazer aqui o WAKEUP hostname
            if(std::regex_search(input, match, regex)){
                
                validInput = true;

                /* wakeonlan é chamado com: 
                std::string wakeonlan = "WAKEONLAN" + mac
                system(wakeonlan.c_str())
                */
                
                auto hostname = match[1].str();
                std::cout << hostname << std::endl;

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
            std::cout << "Nesse ponto, realizaria exit." << std::endl;
            //handleExit();
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
    
//     if (!isManager()){
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
//     g_stop_execution = true;
//     setRunning(false);

// }
