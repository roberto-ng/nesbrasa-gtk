/* controle.hpp
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

#include <array>

#include "memoria.hpp"

namespace nesbrasa::nucleo
{
    using std::array;

    enum class Botao : uint
    {
        A          = 0,
        B          = 1,
        SELECT     = 2,
        START      = 3,
        CIMA       = 4,
        BAIXO      = 5,
        ESQUERDA   = 6,
        DIREITA    = 7,
    };

    class Controle
    {
        array<bool, 8> buffer_botoes;
        byte indice;
        byte sinal;

    public:
        Controle();

        byte ler();
        void escrever(byte valor);

        void set_valor(Botao botao, bool valor);
    };
}