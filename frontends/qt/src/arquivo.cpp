#include "arquivo.hpp"

#include <fstream>
#include <iterator>
#include <stdexcept>

namespace nesbrasa::gui
{
    std::vector<nesbrasa::tipos::byte> ler_arquivo(const std::string& caminho)
    {
        std::ifstream input(caminho, std::ios::binary);
        if (!input)
            throw std::runtime_error("Erro ao abrir arquivo");

        return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    }
}
