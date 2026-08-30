#pragma once
#include <sstream>
#include <iostream>
#include <array>
#include <nesbrasa/tipos_numeros.hpp>
#include <nesbrasa/ports.hpp>

namespace nesbrasa::nucleo
{
    class Memoria : public CpuBus
    {
        std::array<tipos::byte, 0x0800> ram;
        PpuPort* ppu;
        ControllerPort* controle_1;
        ControllerPort* controle_2;
        CartridgePort* cartucho;
        InterruptSink* interrupcoes;

    public:
        Memoria(): ram({ 0 }), ppu(nullptr), controle_1(nullptr), controle_2(nullptr), cartucho(nullptr), interrupcoes(nullptr) {}

        void configurar(PpuPort* ppu, ControllerPort* controle_1, ControllerPort* controle_2, CartridgePort* cartucho, InterruptSink* interrupcoes)
        {
            this->ppu = ppu;
            this->controle_1 = controle_1;
            this->controle_2 = controle_2;
            this->cartucho = cartucho;
            this->interrupcoes = interrupcoes;
        }

        tipos::byte ler(tipos::uint16 endereco) override
        {
            if (endereco <= 0x07FF) return this->ram.at(endereco);
            if (endereco <= 0x1FFF) return this->ram.at(endereco % 0x0800);
            if (endereco <= 0x2007) return this->ppu->registrador_ler(endereco);
            if (endereco <= 0x3FFF) return this->ppu->registrador_ler((endereco % 0x8) + 0x2000);
            if (endereco <= 0x4015) return 0;
            if (endereco == 0x4016) return this->controle_1->ler();
            if (endereco == 0x4017) return this->controle_2->ler();
            if (endereco <= 0x401F) return 0;
            return this->cartucho->ler(endereco);
        }

        tipos::uint16 ler_16_bits(tipos::uint16 endereco) override
        {
            tipos::uint16 menor = this->ler(endereco);
            tipos::uint16 maior = this->ler(endereco + 1);
            return (maior << 8) | menor;
        }

        tipos::uint16 ler_16_bits_bug(tipos::uint16 endereco) override
        {
            tipos::uint16 menor = this->ler(endereco);
            tipos::uint16 maior = 0;
            if ((endereco & 0x00FF) == 0x00FF) maior = this->ler(endereco & 0xFF00);
            else maior = this->ler(endereco + 1);
            return (maior << 8) | menor;
        }

        void escrever(tipos::uint16 endereco, tipos::byte valor) override
        {
            if (endereco <= 0x07FF) this->ram.at(endereco) = valor;
            else if (endereco <= 0x1FFF) this->ram.at(endereco % 0x0800) = valor;
            else if (endereco <= 0x2007) this->ppu->registrador_escrever(endereco, valor);
            else if (endereco <= 0x3FFF) this->ppu->registrador_escrever((endereco % 0x8) + 0x2000, valor);
            else if (endereco <= 0x4017)
            {
                if (endereco == 0x4014) this->ppu->registrador_escrever(endereco, valor);
                else if (endereco == 0x4016) { this->controle_1->escrever(valor); this->controle_2->escrever(valor); }
            }
            else if (endereco >= 0x4020) this->cartucho->escrever(endereco, valor);
        }

        void cpu_ativar_interrupcao(Interrupcao interrupcao)
        {
            this->interrupcoes->ativar_interrupcao(interrupcao);
        }
    };
}

