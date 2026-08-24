/* nesbrasa.hpp
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
#include <vector>
#include <memory>

#include "cpu.hpp"
#include "ppu.hpp"
#include "memoria.hpp"
#include "mapeadores/cartucho.hpp"
#include "controle.hpp"

namespace nesbrasa::nucleo
{
    using std::array;
    using std::vector;
    using std::unique_ptr;
    using namespace mapeadores;

    class Nes
    {
    public:
        Memoria memoria;
        
        Cpu cpu;
        Ppu ppu;
        
        Controle controle_1;
        Controle controle_2;
        
        unique_ptr<Cartucho> cartucho;
        
        bool is_programa_carregado;
        
        Nes();

        void carregar_rom(vector<byte> arquivo);
        int avancar();
    };
}