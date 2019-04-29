#pragma once

#include <vector>
#include <gtkmm.h>

#include "nesbrasa.hpp"

using std::vector;
using nesbrasa::nucleo::Nes;

namespace nesbrasa::gui
{
    vector< vector<guint8> > criar_textura_sprites(Nes& nes);

    vector<guint8> ler_sprite(const Nes& nes, guint pos);
}