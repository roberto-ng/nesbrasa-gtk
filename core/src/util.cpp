#include <nesbrasa/util.hpp>

using namespace nesbrasa::tipos;

namespace nesbrasa::nucleo
{
    bool buscar_bit(byte valor, byte pos)
    {
        // dar a volta quando a posição do bit for maior que 7
        pos = pos % 8;

        const uint8_t tmp = valor & (1 << pos);
        return (tmp >> pos) != 0;
    }

    bool comparar_paginas(tipos::uint16 endereco_1, tipos::uint16 endereco_2)
    {
        return (endereco_1 & 0xFF00) == (endereco_2 & 0xFF00);
    }
}

