module;
#include <array>
#include <optional>
#include <string>
export module nesbrasa.cpu;
import nesbrasa.types;
import nesbrasa.ports;
import nesbrasa.instruction;

export namespace nesbrasa::nucleo
{
    class Cpu : public CpuInterface
    {
        tipos::uint16 esperar;
        tipos::uint32 ciclos;
        std::array<std::optional<Instrucao>, 256> instrucoes;

    public:
        Cpu(CpuBus* memoria);
        tipos::uint avancar();
        void resetar();
        void branch_somar_ciclos(tipos::uint16 endereco);
        tipos::byte get_estado();
        void set_estado(tipos::byte valor);
        void stack_empurrar(tipos::byte valor);
        void stack_empurrar_16_bits(tipos::uint16 valor);
        tipos::byte stack_puxar();
        tipos::uint16 stack_puxar_16_bits();
        void esperar_adicionar(tipos::uint16 esperar);
        std::string instrucao_para_asm(tipos::byte opcode);
        void set_z(tipos::byte valor);
        void set_n(tipos::byte valor);
        tipos::uint32 get_ciclos();
        tipos::uint16 get_esperar();
        std::optional<Instrucao> get_instrucao(tipos::byte opcode);

    private:
        void executar(Instrucao* instrucao);
    };
}
