module;
#include <array>
#include <optional>
#include <string>
export module nesbrasa.instruction;
import nesbrasa.types;

export namespace nesbrasa::nucleo
{
    enum class InstrucaoModo { ACM, ABS, ABS_X, ABS_Y, IMED, IMPL, IND, IND_X, IND_Y, REL, P_ZERO, P_ZERO_X, P_ZERO_Y };
    enum class InstrucaoOperacao {
        ADC, AND, ASL, BCC, BCS, BEQ, BIT, BMI, BNE, BPL, BRK, BVC, BVS,
        CLC, CLD, CLI, CLV, CMP, CPX, CPY, DCP, DEC, DEX, DEY, DOP, EOR,
        INC, INX, INY, ISB, JMP, JSR, LAX, LDA, LDX, LDY, LSR, NOP, ORA,
        PHA, PHP, PLA, PLP, RLA, ROL, ROR, RRA, RTI, RTS, SAX, SBC, SEC,
        SED, SEI, SLO, SRE, STA, STX, STY, TAX, TAY, TOP, TSX, TXA, TXS, TYA
    };
    class Instrucao
    {
    public:
        std::string nome;
        tipos::byte bytes;
        tipos::int32 ciclos;
        tipos::int32 ciclos_pag_alt;
        InstrucaoModo modo;
        InstrucaoOperacao operacao;
        Instrucao(std::string nome, tipos::byte bytes, tipos::int32 ciclos,
                  tipos::int32 ciclos_pag_alt, InstrucaoModo modo,
                  InstrucaoOperacao operacao):
            nome(nome), bytes(bytes), ciclos(ciclos), ciclos_pag_alt(ciclos_pag_alt),
            modo(modo), operacao(operacao) {}
    };
}
