module;
#include <array>
#include <memory>
#include <vector>
export module nesbrasa.nes;
export import nesbrasa.types;
export import nesbrasa.memory;
export import nesbrasa.cpu;
export import nesbrasa.ppu;
export import nesbrasa.controller;
export import nesbrasa.cartridge;

export namespace nesbrasa::nucleo
{
    class Nes
    {
    public:
        static constexpr int TELA_LARGURA = 256;
        static constexpr int TELA_ALTURA = 240;
        static constexpr int CICLOS_POR_QUADRO = 29780;
        Memoria memoria;
        Cpu cpu;
        Ppu ppu;
        Controle controle_1, controle_2;
        std::unique_ptr<mapeadores::Cartucho> cartucho;
        bool is_programa_carregado;
        Nes();
        void carregar_rom(std::vector<tipos::byte> arquivo);
        int avancar();
        int avancar_quadro();
        const std::array<tipos::uint32, 256 * 240>& get_textura() const;
        void set_botao(Botao botao, bool pressionado);
        bool programa_carregado() const;
    };
}
