#pragma once

#include <vector>
#include <gtkmm.h>

#include "nesbrasa.hpp"

using std::vector;
using nesbrasa::nucleo::Nes;

namespace nesbrasa::gui
{
    vector<guint8> criar_textura_sprites(const Nes& nes);
}