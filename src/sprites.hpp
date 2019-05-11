#pragma once

#include <vector>
#include <gtkmm.h>

#include "nesbrasa.hpp"
#include "tipos_numeros.hpp"

namespace nesbrasa::gui
{
    using std::vector;
    using nesbrasa::nucleo::Nes;
    using namespace nesbrasa::tipos;

    vector< vector<byte> > criar_textura_sprites(const Nes& nes);

    vector<byte> ler_sprite(const Nes& nes, guint pos);
}