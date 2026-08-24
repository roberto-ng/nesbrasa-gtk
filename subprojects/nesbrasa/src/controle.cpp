/* controle.cpp
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

#include "controle.hpp"

namespace nesbrasa::nucleo
{
    Controle::Controle()
    {
        this->indice = 0;
        this->sinal = false;
    }

    byte Controle::ler()
    {
        byte valor = 0;
        if (this->indice < 8 && this->buffer_botoes.at(this->indice) == true)
        {
            valor = 1;
        }

        if (this->sinal%1 == 1)
        {
            this->indice = 0;
        }
        else
        {
            this->indice += 1;
        }

        return valor;
    }

    void Controle::escrever(byte valor)
    {
        this->sinal = valor;

        if (this->sinal%1 == 1)
        {
            this->indice = 0;
        }
    }

    void Controle::set_valor(Botao botao, bool valor)
    {
        uint botao_indice = static_cast<uint>(botao);
        this->buffer_botoes.at(botao_indice) = valor;
    }
}