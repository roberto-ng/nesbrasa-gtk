module;
export module nesbrasa.ports;

import nesbrasa.types;

export namespace nesbrasa::nucleo
{
    struct CpuBus { virtual tipos::byte ler(tipos::uint16) = 0; virtual tipos::uint16 ler_16_bits(tipos::uint16) = 0; virtual tipos::uint16 ler_16_bits_bug(tipos::uint16) = 0; virtual void escrever(tipos::uint16, tipos::byte) = 0; virtual ~CpuBus() = default; };
    struct PpuPort { virtual tipos::byte registrador_ler(tipos::uint16) = 0; virtual void registrador_escrever(tipos::uint16, tipos::byte) = 0; virtual ~PpuPort() = default; };
    struct ControllerPort { virtual tipos::byte ler() = 0; virtual void escrever(tipos::byte) = 0; virtual ~ControllerPort() = default; };
    struct CartridgePort { virtual tipos::byte ler(tipos::uint16) = 0; virtual void escrever(tipos::uint16, tipos::byte) = 0; virtual tipos::byte get_espelhamento() const = 0; virtual ~CartridgePort() = default; };
    struct InterruptSink { virtual void ativar_interrupcao(Interrupcao) = 0; virtual ~InterruptSink() = default; };
    struct DmaSink { virtual void solicitar_dma(tipos::byte) = 0; virtual ~DmaSink() = default; };

    struct CpuInterface
    {
        CpuBus* memoria;
        Interrupcao interrupcao;
        tipos::uint16 pc;
        tipos::byte sp, a, x, y;
        bool c, z, i, d, b, v, n, is_pag_alterada;
        virtual void branch_somar_ciclos(tipos::uint16) = 0;
        virtual tipos::byte get_estado() = 0;
        virtual void set_estado(tipos::byte) = 0;
        virtual void stack_empurrar(tipos::byte) = 0;
        virtual void stack_empurrar_16_bits(tipos::uint16) = 0;
        virtual tipos::byte stack_puxar() = 0;
        virtual tipos::uint16 stack_puxar_16_bits() = 0;
        virtual void esperar_adicionar(tipos::uint16) = 0;
        virtual tipos::uint32 get_ciclos() = 0;
        virtual void set_z(tipos::byte) = 0;
        virtual void set_n(tipos::byte) = 0;
        virtual ~CpuInterface() = default;
    };
}
