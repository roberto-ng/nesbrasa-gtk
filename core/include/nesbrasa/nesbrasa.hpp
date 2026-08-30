#pragma once
#include <nesbrasa/controle.hpp>
#include <nesbrasa/memoria.hpp>
#include <nesbrasa/cpu.hpp>
#include <nesbrasa/ppu.hpp>
#include <string>
#include <sstream>
#include <iostream>
#include <array>
#include <memory>
#include <vector>
#include <nesbrasa/util.hpp>
#include <nesbrasa/mapeadores/nrom.hpp>
#include <nesbrasa/ports.hpp>

namespace nesbrasa::nucleo
{
    class Nes : public InterruptSink, public DmaSink
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
        void ativar_interrupcao(Interrupcao) override;
        void solicitar_dma(tipos::byte) override;
    };
}

