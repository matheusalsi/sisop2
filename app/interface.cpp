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

    //std::thread inputThread(&InterfaceSS::inputThread, this);

    while(isRunning()){
        printInterface();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    //inputThread.join();

}

void InterfaceSS::printInterface(){

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

}

// void InterfaceSS::inputThread(){
//     std::string input;
//     std::regex regex("wakeup (.*)");
//     std::smatch match;
//     while(isRunning()){
//         std::getline(std::cin, input); // Pega espaços
//         if (isManager()){ // Fazer aqui o WAKEUP hostname
//             if(std::regex_search(input, match, regex)){
//                 /* wakeonlan é chamado com: 
//                 std::string wakeonlan = "WAKEONLAN" + mac
//                 system(wakeonlan.c_str())
//                 */
                
//                 auto hostname = match[0].str();
//                 auto mac = localTable.getMacFromHostname(hostname);
//                 std::string wakeonlan = "wakeonlan ";
//                 wakeonlan.append(mac);
                
//                 std::cout << "Enviando comando \"" << wakeonlan << "\"" << std::endl;

//                 system(wakeonlan.c_str());

//             }
//         }
//         else if (input == "EXIT"){
//             handleExit();
//         }
//     }
    
// }

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
