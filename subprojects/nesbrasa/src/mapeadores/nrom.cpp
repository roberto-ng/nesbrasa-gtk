/* nrom.cpp
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

#include <stdexcept>

#include "nrom.hpp"

namespace nesbrasa::nucleo::mapeadores
{
    using std::runtime_error;
    using namespace std::string_literals;
    using namespace nucleo;

    NRom::NRom(int prg_bancos_qtd, int chr_bancos_qtd, 
               vector<byte>& arquivo, ArquivoFormato formato,
               byte espelhamento):
        Cartucho(prg_bancos_qtd, chr_bancos_qtd, arquivo, formato, espelhamento)
    {
        // aloca a memória que representará a ram PRG
        this->ram_prg.resize(0x2000);

        if (this->possui_chr_ram)
        {
            // aloca a memória que representará a ram CHR
            this->ram_chr.resize(0x2000);
        }
    }

    uint8_t NRom::ler(uint16 endereco)
    {
        if (endereco < 0x2000)
        {
            // ler a rom CHR ou a ram CHR
            if (this->possui_chr_ram)
            {
                return ram_chr.at(endereco);
            }
            else
            {
                return rom_chr.at(endereco);   
            }
        }
        else if (endereco >= 0x8000)
        {
            // os bancos da rom PRG começam a partir do endereço 0x8000
            uint16_t endereco_mapeado = endereco - 0x8000;

            // espelhar o endereço caso a rom PRG só possua 1 banco
            if (this->prg_bancos_quantidade == 1)
            {
                return this->rom_prg.at(endereco_mapeado % 0x4000);
            }
            else
            {
                return this->rom_prg.at(endereco_mapeado);
            }
        }

        return 0;
    }

    void NRom::escrever(uint16 endereco, byte valor)
    {
        if (!this->possui_chr_ram)
        {
            throw runtime_error("CHR RAM inexistente"s);
        }

        if (endereco < 0x2000)
        {
            // escrever na rom CHR
            this->rom_chr.at(endereco) = valor;
        }
    }

    string NRom::get_nome()
    {
        return "NROM";
    }
}