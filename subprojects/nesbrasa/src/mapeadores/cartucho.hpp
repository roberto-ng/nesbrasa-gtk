/* cartucho.hpp
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

#pragma once

#include <string>
#include <vector>
#include <memory>

#include "tipos_numeros.hpp"

namespace nesbrasa::nucleo::mapeadores
{
    using std::string;
    using std::vector;
    using std::unique_ptr;
    using namespace tipos;

    enum class CartuchoTipo
    {
        NROM = 0,
        MMC1 = 1,
        DESCONHECIDO,
    };

    enum class ArquivoFormato
    {
        DESCONHECIDO,
        INES,
        NES_2_0,
    };
    
    //! Classe abstrata definindo padrão para diferentes tipos de cartuchos
    class Cartucho
    {
    protected:
        // quantidade de bancos da ROM PRG
        int prg_bancos_quantidade;
        // quantidade de bancos da ROM CHR
        int chr_bancos_quantidade;

        bool possui_prg_ram;
        bool possui_chr_ram;

        vector<byte> rom_prg;
        vector<byte> rom_chr;
        vector<byte> ram_prg;
        vector<byte> ram_chr;
    
    public:
        ArquivoFormato arquivo_formato;

        byte espelhamento;

        // tamanho em bytes de um banco da ROM PRG
        static const int PRG_BANCOS_TAMANHO;
        // tamanho em bytes de um banco da ROM CHR
        static const int CHR_BANCOS_TAMANHO;    

        // método factory
        static unique_ptr<Cartucho> criar(CartuchoTipo tipo, int prg_qtd, int chr_qtd, 
                                          vector<byte>& arquivo, ArquivoFormato formato,
                                          byte espelhamento);

        Cartucho(int prg_bancos_qtd, int chr_bancos_qtd, 
                 vector<byte>& arquivo, ArquivoFormato formato, 
                 byte espelhamento);

        virtual ~Cartucho() = default;

        virtual byte ler(uint16 endereco) = 0;
        virtual void escrever(uint16 endereco, byte valor) = 0;

        virtual string get_nome() = 0;

        int get_prg_bancos_quantidade();
        int get_chr_bancos_quantidade();
    };
}