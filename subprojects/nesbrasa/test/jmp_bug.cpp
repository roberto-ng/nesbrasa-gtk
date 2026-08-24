#include <memory>

#include "nesbrasa.hpp"
#include "memoria.hpp"

using nesbrasa::nucleo::Nes;
using std::unique_ptr;
using std::make_unique;

int main()
{
    // testa a implementação de um bug

    auto nes = make_unique<Nes>();
    nes->memoria.escrever(0x0FFF, 0x05);
    nes->memoria.escrever(0x0F00, 0xF3);

    auto valor = nes->memoria.ler_16_bits_bug(0x0FFF);

    if (valor == 0xF305)
    {
        return EXIT_SUCCESS;
    }
    else
    {
        return EXIT_FAILURE;
    }

}
