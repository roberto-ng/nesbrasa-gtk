module;
export module nesbrasa.ports;

import nesbrasa.types;

export namespace nesbrasa::nucleo
{
    struct CpuBus
    {
        virtual tipos::byte ler(tipos::uint16 endereco) = 0;
        virtual tipos::uint16 ler_16_bits(tipos::uint16 endereco) = 0;
        virtual tipos::uint16 ler_16_bits_bug(tipos::uint16 endereco) = 0;
        virtual void escrever(tipos::uint16 endereco, tipos::byte valor) = 0;
        virtual ~CpuBus() = default;
    };

    struct PpuPort
    {
        virtual tipos::byte registrador_ler(tipos::uint16 endereco) = 0;
        virtual void registrador_escrever(tipos::uint16 endereco, tipos::byte valor) = 0;
        virtual ~PpuPort() = default;
    };

    struct ControllerPort
    {
        virtual tipos::byte ler() = 0;
        virtual void escrever(tipos::byte valor) = 0;
        virtual ~ControllerPort() = default;
    };

    struct CartridgePort
    {
        virtual tipos::byte ler(tipos::uint16 endereco) = 0;
        virtual void escrever(tipos::uint16 endereco, tipos::byte valor) = 0;
        virtual tipos::byte get_espelhamento() const = 0;
        virtual ~CartridgePort() = default;
    };

    struct InterruptSink
    {
        virtual void ativar_interrupcao(Interrupcao interrupcao) = 0;
        virtual ~InterruptSink() = default;
    };

    struct DmaSink
    {
        virtual void solicitar_dma(tipos::byte pagina) = 0;
        virtual ~DmaSink() = default;
    };
}
