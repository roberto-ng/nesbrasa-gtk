module;
#include <sstream>
#include <iostream>
#include <array>
export module nesbrasa.memory;
import nesbrasa.types;
import nesbrasa.ports;

export namespace nesbrasa::nucleo
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
        Memoria();
        void configurar(PpuPort*, ControllerPort*, ControllerPort*, CartridgePort*, InterruptSink*);
        tipos::byte ler(tipos::uint16 endereco) override;
        tipos::uint16 ler_16_bits(tipos::uint16 endereco) override;
        tipos::uint16 ler_16_bits_bug(tipos::uint16 endereco) override;
        void escrever(tipos::uint16 endereco, tipos::byte valor) override;
        void cpu_ativar_interrupcao(Interrupcao interrupcao);
    };
}


using namespace nesbrasa::tipos;

namespace nesbrasa::nucleo
{
    using std::stringstream;
    using std::runtime_error;

    Memoria::Memoria():
        ram({ 0 }), ppu(nullptr), controle_1(nullptr), controle_2(nullptr), cartucho(nullptr), interrupcoes(nullptr)
    {
    }

    void Memoria::configurar(PpuPort* ppu, ControllerPort* controle_1, ControllerPort* controle_2, CartridgePort* cartucho, InterruptSink* interrupcoes)
    {
        this->ppu = ppu;
        this->controle_1 = controle_1;
        this->controle_2 = controle_2;
        this->cartucho = cartucho;
        this->interrupcoes = interrupcoes;
    }

    byte Memoria::ler(uint16 endereco)
    {
        if (endereco <= 0x07FF)
        {
            return this->ram.at(endereco);
        }
        else if (endereco >= 0x0800 && endereco <=0x1FFF)
        {
            // endereços nesta area são espelhos dos endereços
            // localizados entre 0x0000 e 0x07FF
            return this->ram.at(endereco % 0x0800);
        }
        else if (endereco >= 0x2000 && endereco <= 0x2007)
        {
            return this->ppu->registrador_ler(endereco);
        }
        else if (endereco >= 0x2008 && endereco <= 0x3FFF)
        {
            // endereço espelhado do registrador
            uint16 ender_espelhado = (endereco%0x8) + 0x2000;
            return this->ppu->registrador_ler(ender_espelhado);
        }
        else if (endereco >= 0x4000 && endereco <= 0x4015)
        {
            // TODO: registradores da APU
            return 0;
        }
        else if (endereco == 0x4016)
        {
            return this->controle_1->ler();
        }
        else if (endereco == 0x4017)
        {
            return this->controle_2->ler();
        }
        else if (endereco >= 0x4018 && endereco <= 0x401F)
        {
            // endereços não útilizados 
            return 0;
        }
        else if (endereco >= 0x4020 && endereco <= 0xFFFF)
        {
            return this->cartucho->ler(endereco);
        }

        // endereço não existe, lançar erro
        stringstream erro_ss;
        erro_ss << "Tentativa de leitura em um endereço não existente na memória";
        erro_ss << " (" << std::hex << endereco << ") ";
        erro_ss << endereco;

        throw runtime_error(erro_ss.str());
    }

    uint16 Memoria::ler_16_bits(uint16 endereco)
    {
        uint16 menor = this->ler(endereco);
        uint16 maior = this->ler(endereco + 1);

        return (maior << 8) | menor;
    }

    uint16 Memoria::ler_16_bits_bug(uint16 endereco)
    {
        uint16 menor = this->ler(endereco);
        uint16 maior = 0;

        if ((endereco & 0x00FF) == 0x00FF)
        {
            maior = this->ler(endereco & 0xFF00);
        }
        else
        {
            maior = this->ler(endereco + 1);
        }

        return (maior << 8) | menor;
    }

    void Memoria::escrever(uint16 endereco, byte valor)
    {
        if (endereco <= 0x07FF)
        {
            this->ram.at(endereco) = valor;
        }
        else if (endereco >= 0x0800 && endereco <=0x1FFF)
        {
            // endereços nesta area são espelhos dos endereços
            // localizados entre 0x0000 e 0x07FF
            this->ram.at(endereco % 0x0800) = valor;
        }
        else if (endereco >= 0x2000 && endereco <= 0x2007)
        {
            this->ppu->registrador_escrever(endereco, valor);
        }
        else if (endereco >= 0x2008 && endereco <= 0x3FFF)
        {
            // endereço espelhado do registrador
            uint16 ender_espelhado = (endereco%0x8) + 0x2000;
            this->ppu->registrador_escrever(ender_espelhado, valor);
        }
        else if (endereco >= 0x4000 && endereco <= 0x4017)
        {
            // $4014 is the PPU OAM DMA register, not an APU register.
            if (endereco == 0x4014)
            {
                this->ppu->registrador_escrever(endereco, valor);
            }
            else if (endereco == 0x4016)
            {
                this->controle_1->escrever(valor);
                this->controle_2->escrever(valor);
            }
            else 
            {
                // TODO: registradores da APU
            }
        }
        else if (endereco >= 0x4018 && endereco <= 0x401F)
        {
            // endereços não útilizados
            return;
        }
        else if (endereco >= 0x4020 && endereco <= 0xFFFF)
        {
            this->cartucho->escrever(endereco, valor);
        }
        else
        {
            // endereço não existe, lançar erro
            stringstream erro_ss;
            erro_ss << "Tentativa de escrita do valor " << std::hex << valor;
            erro_ss << " em um endereço não existente na memória";
            erro_ss << " (" << std::hex << endereco << ")";

            throw runtime_error(erro_ss.str());
        }
    }

    void Memoria::cpu_ativar_interrupcao(Interrupcao interrupcao)
    {
        this->interrupcoes->ativar_interrupcao(interrupcao);
    }
}
