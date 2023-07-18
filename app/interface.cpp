#include "interface.h"

void InterfaceSS::userInputInterface(){
    
    std::string input;
    
    std::cout << "Digite o comando: " << std::endl;
    std::cin >> input;

    if (input == "WAKEUP" && isManager())
        ; // parsear para WAKEONLAN
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

// void InterfaceSS::printInterface(){
//     std::system("clear"); // Limpa tela (linux)
//     table.printToConsole(); // Exibe tabela
// }