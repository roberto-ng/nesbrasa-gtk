#include "arquivo.hpp"

namespace nesbrasa::gui
{
    vector<uint8_t> ler_arquivo(string caminho)
    {
        ifstream input(caminho, std::ios::binary);

        if (input.fail())
        {
            throw -1;
        }
    
        return vector<uint8_t>(
            std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>() 
        );
    }
}