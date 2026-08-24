/* memoria.hpp
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

#include <cstdint>
#include <array>

#include "tipos_numeros.hpp"

namespace nesbrasa::nucleo
{
    using std::array;
    using namespace tipos;
        
    class Nes;
    enum class Interrupcao;

    class Memoria
    {
    private:
        array<byte, 0x0800> ram; 
        
    public:        
        Nes* nes;
        
        Memoria(Nes* nes);

        //! Lê um valor de 8 bits na memoria
        byte ler(uint16 endereco);

        //! Lê um valor de 16 bits na memoria
        uint16 ler_16_bits(uint16 endereco);

        //! Lê um valor de 16 bits na memoria do no modo indireto da cpu reproduzindo um bug da CPU
        uint16 ler_16_bits_bug(uint16 endereco);

        //! Escreve um valor na memoria
        void escrever(uint16 endereco, byte valor);

        void cpu_ativar_interrupcao(Interrupcao interrupcao);
    };
}