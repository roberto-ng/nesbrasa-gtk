module;
#include <array>
namespace nesbrasa::nucleo { class Nes; }
export module nesbrasa.memory;
import nesbrasa.types;

export namespace nesbrasa::nucleo
{
    class Memoria
    {
        std::array<tipos::byte, 0x0800> ram;

    public:
        Nes* nes;
        Memoria(Nes* nes);
        tipos::byte ler(tipos::uint16 endereco);
        tipos::uint16 ler_16_bits(tipos::uint16 endereco);
        tipos::uint16 ler_16_bits_bug(tipos::uint16 endereco);
        void escrever(tipos::uint16 endereco, tipos::byte valor);
        void cpu_ativar_interrupcao(Interrupcao interrupcao);
    };
}
