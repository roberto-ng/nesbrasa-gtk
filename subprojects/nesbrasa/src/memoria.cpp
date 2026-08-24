/* memoria.cpp
 *
 * Copyright 2019 Roberto Nazareth <nazarethroberto97@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <sstream>
#include <iostream>

#include "memoria.hpp"
#include "nesbrasa.hpp"

namespace nesbrasa::nucleo
{
    using std::stringstream;
    using std::runtime_error;

    Memoria::Memoria(Nes *nes):
        ram({ 0 })
    {
        this->nes = nes;
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
            return this->nes->ppu.registrador_ler(endereco);
        }
        else if (endereco >= 0x2008 && endereco <= 0x3FFF)
        {
            // endereço espelhado do registrador
            uint16 ender_espelhado = (endereco%0x8) + 0x2000;
            return this->nes->ppu.registrador_ler(ender_espelhado);
        }
        else if (endereco >= 0x4000 && endereco <= 0x4015)
        {
            // TODO: registradores da APU
            return 0;
        }
        else if (endereco == 0x4016)
        {
            return this->nes->controle_1.ler();
        }
        else if (endereco == 0x4017)
        {
            return this->nes->controle_2.ler();
        }
        else if (endereco >= 0x4018 && endereco <= 0x401F)
        {
            // endereços não útilizados 
            return 0;
        }
        else if (endereco >= 0x4020 && endereco <= 0xFFFF)
        {
            return this->nes->cartucho->ler(endereco);
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
            this->nes->ppu.registrador_escrever(nes, endereco, valor);
        }
        else if (endereco >= 0x2008 && endereco <= 0x3FFF)
        {
            // endereço espelhado do registrador
            uint16 ender_espelhado = (endereco%0x8) + 0x2000;
            this->nes->ppu.registrador_escrever(nes, ender_espelhado, valor);
        }
        else if (endereco >= 0x4000 && endereco <= 0x4017)
        {
            if (endereco == 0x4016)
            {
                this->nes->controle_1.escrever(valor);
                this->nes->controle_2.escrever(valor);
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
            this->nes->cartucho->escrever(endereco, valor);
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
        this->nes->cpu.interrupcao = interrupcao;
    }
}