module;

#include <stdexcept>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

module nesbrasa.gtk;

namespace nesbrasa::gui
{
    using std::runtime_error;
    using namespace std::string_literals;

    std::vector<nesbrasa::tipos::byte> ler_arquivo(std::string caminho)
    {
        std::ifstream input(caminho, std::ios::binary);

        if (input.fail())
        {
            throw std::runtime_error("Erro ao abrir arquivo");
        }
    
        return std::vector<nesbrasa::tipos::byte>(
            std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>()
        );
    }
}
