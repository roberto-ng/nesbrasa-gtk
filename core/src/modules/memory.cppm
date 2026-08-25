module;
#include <array>
export module nesbrasa.memory;
import nesbrasa.types;
import nesbrasa.ports;

export namespace nesbrasa::nucleo
{
    class Memoria : public CpuBus
    {
        std::array<tipos::byte, 0x0800> ram;

        PpuPort* ppu;
        ControllerPort* controle_1;
        ControllerPort* controle_2;
        CartridgePort* cartucho;
        InterruptSink* interrupcoes;
    public:
        Memoria();
        void configurar(PpuPort*, ControllerPort*, ControllerPort*, CartridgePort*, InterruptSink*);
        tipos::byte ler(tipos::uint16 endereco) override;
        tipos::uint16 ler_16_bits(tipos::uint16 endereco) override;
        tipos::uint16 ler_16_bits_bug(tipos::uint16 endereco) override;
        void escrever(tipos::uint16 endereco, tipos::byte valor) override;
        void cpu_ativar_interrupcao(Interrupcao interrupcao);
    };
}
