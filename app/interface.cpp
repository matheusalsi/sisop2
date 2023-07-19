#include "interface.h"

void InterfaceSS::userInputInterface(){
    
    std::string input;
    
    std::cout << "Digite o comando: " << std::endl;
    std::cin >> input;

    if (input == "" && isManager()){ // Fazer aqui o WAKEUP hostname
        /* wakeonlan é chamado com: 
        std::string wakeonlan = "WAKEONLAN" + mac
        system(wakeonlan.c_str())
        */
       ;
    }
    else if (input == "EXIT"){
        if (!isManager()){
            mailBox.writeMessage("D_IN <- I_OUT", input); // Participante avisa para o seu discovery que vai sair
            while(mailBox.isEmpty("D_OUT -> I_IN")){ // Manager espera o seu discovery avisar que o participante foi removido da tabela
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
    while(isRunning()){
        
        //userInputInterface();
        // printInterface();

    }
}

// void InterfaceSS::printInterface(){
//     std::system("clear"); // Limpa tela (linux)
//     table.printToConsole(); // Exibe tabela
// }