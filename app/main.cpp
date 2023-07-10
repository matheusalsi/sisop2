#include "subsystems.h"
#include <string>

int main(int argc, char *argv[])
{

    InterfaceSS interfaceSS(false);
    interfaceSS.start();

    // Exemplo de parada do SS por finalização da thread principal
    std::string input;
    std::cin >> input;

    interfaceSS.stop();
    std::cout << "Aqui está sua entrada: " << input << std::endl;

    return 0;
}