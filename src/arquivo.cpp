#include "arquivo.hpp"

#include <stdexcept>

using std::runtime_error;

namespace nesbrasa::gui
{
    using namespace std::string_literals;

    vector<uint8_t> ler_arquivo(string caminho)
    {
        ifstream input(caminho, std::ios::binary);

        if (input.fail())
        {
            throw runtime_error("Erro ao abrir arquivo"s);
        }
    
        return vector<uint8_t>(
            std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>() 
        );
    }
}