module;
#include <array>
#include <functional>
#include <optional>
#include <string>
export module nesbrasa.instruction;
import nesbrasa.types;
import nesbrasa.ports;

export namespace nesbrasa::nucleo
{
    class Instrucao;
    using InstrucaoImplementacao = std::function<void(Instrucao*, CpuInterface*, std::optional<tipos::uint16>)>;

    enum class InstrucaoModo { ACM, ABS, ABS_X, ABS_Y, IMED, IMPL, IND, IND_X, IND_Y, REL, P_ZERO, P_ZERO_X, P_ZERO_Y };

    class Instrucao
    {
    public:
        std::string nome;
        tipos::byte bytes;
        tipos::int32 ciclos;
        tipos::int32 ciclos_pag_alt;
        InstrucaoModo modo;
        InstrucaoImplementacao implementacao;

        Instrucao(std::string nome, tipos::byte bytes, tipos::int32 ciclos,
                  tipos::int32 ciclos_pag_alt, InstrucaoModo modo,
                  InstrucaoImplementacao implementacao);
        std::optional<tipos::uint16> buscar_endereco(CpuInterface* cpu);
    };

    std::array<std::optional<Instrucao>, 256> carregar_instrucoes();
}
