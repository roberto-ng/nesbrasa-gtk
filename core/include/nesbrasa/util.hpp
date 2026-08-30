#pragma once
#include <nesbrasa/tipos_numeros.hpp>

namespace nesbrasa::nucleo
{
    bool buscar_bit(tipos::byte valor, tipos::byte pos);
    bool comparar_paginas(tipos::uint16 pagina_1, tipos::uint16 pagina_2);
}

