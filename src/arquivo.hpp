#pragma once

#include <vector>
#include <string>
#include <fstream>

#include "tipos_numeros.hpp"

namespace nesbrasa::gui
{
    using std::vector;
    using std::string;
    using std::ifstream;
    using namespace nesbrasa::tipos;

    vector<byte> ler_arquivo(string caminho);
}