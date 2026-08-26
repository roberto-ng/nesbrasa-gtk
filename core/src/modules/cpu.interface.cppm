export module nesbrasa.cpu.interface;

import nesbrasa.types;
import nesbrasa.ports;

export namespace nesbrasa::nucleo
{
    struct CpuInterface
    {
        CpuBus* memoria;
        Interrupcao interrupcao;
        tipos::uint16 pc;
        tipos::byte sp, a, x, y;
        bool c, z, i, d, b, v, n, is_pag_alterada;

        virtual void branch_somar_ciclos(tipos::uint16 endereco) = 0;
        virtual tipos::byte get_estado() = 0;
        virtual void set_estado(tipos::byte valor) = 0;
        virtual void stack_empurrar(tipos::byte valor) = 0;
        virtual void stack_empurrar_16_bits(tipos::uint16 valor) = 0;
        virtual tipos::byte stack_puxar() = 0;
        virtual tipos::uint16 stack_puxar_16_bits() = 0;
        virtual void esperar_adicionar(tipos::uint16 ciclos) = 0;
        virtual tipos::uint32 get_ciclos() = 0;
        virtual void set_z(tipos::byte valor) = 0;
        virtual void set_n(tipos::byte valor) = 0;
        virtual ~CpuInterface() = default;
    };
}
