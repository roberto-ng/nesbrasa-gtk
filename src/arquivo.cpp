#include "arquivo.hpp"

#include <stdexcept>

namespace nesbrasa::gui
{
    using std::runtime_error;
    using namespace std::string_literals;

    vector<byte> ler_arquivo(string caminho)
    {
        ifstream input(caminho, std::ios::binary);

        if (input.fail())
        {
            throw runtime_error("Erro ao abrir arquivo"s);
        }
    
        return vector<byte>(
            std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>() 
        );
    }
}