#include <emscripten/bind.h>

#include <cstdint>
#include <vector>

#include <nesbrasa.hpp>

namespace
{
    using nesbrasa::nucleo::Botao;
    using nesbrasa::nucleo::Nes;
    using nesbrasa::tipos::byte;

    class WebNes
    {
        Nes nes;

    public:
        void carregar_rom(emscripten::val dados)
        {
            const unsigned length = dados["length"].as<unsigned>();
            std::vector<byte> rom(length);
            for (unsigned i = 0; i < length; ++i)
            {
                rom[i] = dados[i].as<byte>();
            }
            nes.carregar_rom(std::move(rom));
        }

        int avancar_quadro() { return nes.avancar_quadro(); }

        std::uintptr_t framebuffer_ptr() const
        {
            return reinterpret_cast<std::uintptr_t>(nes.get_textura().data());
        }

        bool programa_carregado() const { return nes.programa_carregado(); }

        void set_botao(int botao, bool pressionado)
        {
            nes.set_botao(static_cast<Botao>(botao), pressionado);
        }
    };
}

EMSCRIPTEN_BINDINGS(nesbrasa_web)
{
    emscripten::class_<WebNes>("WebNes")
        .constructor<>()
        .function("carregar_rom", &WebNes::carregar_rom)
        .function("avancar_quadro", &WebNes::avancar_quadro)
        .function("framebuffer_ptr", &WebNes::framebuffer_ptr)
        .function("programa_carregado", &WebNes::programa_carregado)
        .function("set_botao", &WebNes::set_botao);
}
