#include <cstdint>
#include <vector>

#include <nesbrasa.hpp>

using nesbrasa::nucleo::Botao;
using nesbrasa::nucleo::Nes;
using nesbrasa::tipos::byte;

extern "C"
{
    Nes* nes_create() { return new Nes(); }
    void nes_destroy(Nes* nes) { delete nes; }

    void nes_load_rom(Nes* nes, const std::uint8_t* data, std::size_t length)
    {
        nes->carregar_rom(std::vector<byte>(data, data + length));
    }

    int nes_avancar_quadro(Nes* nes) { return nes->avancar_quadro(); }

    const std::uint32_t* nes_framebuffer(const Nes* nes)
    {
        return nes->get_textura().data();
    }

    bool nes_programa_carregado(const Nes* nes)
    {
        return nes->programa_carregado();
    }

    void nes_set_botao(Nes* nes, int botao, bool pressionado)
    {
        nes->set_botao(static_cast<Botao>(botao), pressionado);
    }
}
