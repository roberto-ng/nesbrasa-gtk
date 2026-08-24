/* cartucho.cpp
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
#include <sstream>

#include "cartucho.hpp"
#include "nrom.hpp"
#include "util.hpp"

namespace nesbrasa::nucleo::mapeadores
{
    using std::runtime_error;
    using std::stringstream;
    using std::make_unique;
    using namespace std::string_literals;

    const int Cartucho::PRG_BANCOS_TAMANHO = 0x4000;
    const int Cartucho::CHR_BANCOS_TAMANHO = 0x2000;

    unique_ptr<Cartucho> Cartucho::criar(CartuchoTipo tipo, int prg_qtd, int chr_qtd, 
                                         vector<byte>& arquivo, ArquivoFormato formato,
                                         byte espelhamento)
    {
        switch (tipo)
        {
            case CartuchoTipo::NROM:
                return make_unique<NRom>(prg_qtd, chr_qtd, arquivo, formato, espelhamento);
                break;

            default:
            {
                // lançar mensagem de erro
                stringstream erro_ss;
                erro_ss << "Erro: mapeador não reconhecido\n";
                erro_ss << "Código do mapeador: " << static_cast<int>(tipo);
                throw runtime_error(erro_ss.str());
            }
        }
    }

    Cartucho::Cartucho(int prg_bancos_qtd, 
                 int chr_bancos_qtd, 
                 vector<byte>& arquivo,
                 ArquivoFormato formato,
                 byte espelhamento)
    {
        this->espelhamento = espelhamento;
        this->arquivo_formato = formato;
        this->prg_bancos_quantidade = prg_bancos_qtd;
        this->chr_bancos_quantidade = chr_bancos_qtd;

        this->possui_prg_ram = buscar_bit(arquivo.at(6), 1);

        this->possui_chr_ram = false;
        if (chr_bancos_qtd == 0)
        {
            this->possui_chr_ram = true;
        }

        uint rom_prg_tamanho = prg_bancos_qtd * this->PRG_BANCOS_TAMANHO;
        uint rom_chr_tamanho = chr_bancos_qtd * this->CHR_BANCOS_TAMANHO;

        // aloca a memória necessária
        this->rom_prg.resize(rom_prg_tamanho);
        this->rom_chr.resize(rom_chr_tamanho);

        // busca o inicio da ROM PRG
        int rom_prg_inicio = 0;
        if (buscar_bit(arquivo.at(6), 2) == true)
        {
            rom_prg_inicio = 16 + 512;
        }
        else
        {
            rom_prg_inicio = 16;
        }

        // checa o tamanho do arquivo
        if ((rom_prg_inicio + rom_prg_tamanho + rom_chr_tamanho) > arquivo.size())
        {
            // formato inválido
            throw runtime_error("Erro: formato não reconhecido"s);
        }

        // calcula o inicio da ROM CHR
        int rom_chr_inicio = rom_prg_inicio + rom_prg_tamanho;

        // Copia os dados referentes à ROM PRG do arquivo para o array
        for (uint i = 0; i < this->rom_prg.size(); i++)
        {
            this->rom_prg.at(i) = arquivo.at(rom_prg_inicio+i);
        }

        // Copia os dados referentes à ROM CHR do arquivo para o array
        for (uint i = 0; i < this->rom_chr.size(); i++) 
        {
            this->rom_chr.at(i) = arquivo.at(rom_chr_inicio+i);
        }
    }

    int Cartucho::get_prg_bancos_quantidade()
    {
        return this->prg_bancos_quantidade;
    }

    int Cartucho::get_chr_bancos_quantidade()
    {
        return this->chr_bancos_quantidade;
    }
}