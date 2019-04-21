#pragma once

#include <vector>
#include <string>
#include <fstream>

using std::vector;
using std::string;
using std::ifstream;

namespace nesbrasa::gui
{
    vector<uint8_t> ler_arquivo(string caminho);
}