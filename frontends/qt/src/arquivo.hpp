#pragma once

#include <nesbrasa/tipos_numeros.hpp>

#include <string>
#include <vector>

namespace nesbrasa::gui
{
    std::vector<nesbrasa::tipos::byte> ler_arquivo(const std::string& caminho);
}
