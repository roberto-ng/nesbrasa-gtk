/* util.cpp
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

#include "util.hpp"

namespace nesbrasa::nucleo
{
    bool buscar_bit(byte valor, byte pos)
    {
        // dar a volta quando a posição do bit for maior que 7
        pos = pos % 8;

        const uint8_t tmp = valor & (1 << pos);
        return (tmp >> pos) != 0;
    }

    bool comparar_paginas(uint16_t endereco_1, uint16_t endereco_2)
    {
        return (endereco_1 & 0xFF00) == (endereco_2 & 0xFF00);
    }
}