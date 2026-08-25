#include <cstdlib>
#include <vector>

import nesbrasa;

using nesbrasa::nucleo::Botao;
using nesbrasa::nucleo::Nes;
using nesbrasa::tipos::byte;

int main()
{
    Nes nes;

    if (nes.get_textura().size() != Nes::TELA_LARGURA * Nes::TELA_ALTURA)
    {
        return EXIT_FAILURE;
    }

    std::vector<byte> rom(16 + 0x4000 + 0x2000, 0);
    rom[0] = 'N';
    rom[1] = 'E';
    rom[2] = 'S';
    rom[3] = 0x1A;
    rom[4] = 1;
    rom[5] = 1;

    nes.carregar_rom(std::move(rom));
    nes.set_botao(Botao::A, true);
    nes.avancar_quadro();

    return nes.programa_carregado() ? EXIT_SUCCESS : EXIT_FAILURE;
}
