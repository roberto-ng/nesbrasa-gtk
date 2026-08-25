module;
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <array>
#include <optional>
#include <string>
export module nesbrasa.cpu;
import nesbrasa.util;
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


using namespace nesbrasa::tipos;
using std::optional;
using std::string;

namespace nesbrasa::nucleo
{
    using std::stringstream;
    using std::runtime_error;

    Cpu::Cpu(CpuBus* memoria):
        instrucoes(carregar_instrucoes())
    {
        this->memoria = memoria;
        this->ciclos = 0;
        this->pc = 0;
        this->sp = 0;
        this->a = 0;
        this->x = 0;
        this->y = 0;
        this->c = false;
        this->z = false;
        this->i = false;
        this->d = false;
        this->b = false;
        this->v = false;
        this->n = false;
        this->esperar = 0;
        this->is_pag_alterada = false;
        this->interrupcao = Interrupcao::NENHUMA;
    }

    uint Cpu::avancar()
    {
        if (this->esperar > 0)
        {
            this->esperar -= 1;
            return 1;
        }

        uint ciclos = this->ciclos;

        switch (this->interrupcao)
        {
            case Interrupcao::NMI:
                this->stack_empurrar_16_bits(this->pc);
                this->stack_empurrar(this->get_estado() | 0x10);
                this->pc = this->memoria->ler_16_bits(0xFFFA);
                this->i = 1;
                this->ciclos += 7;
                break;
            
            case Interrupcao::IRQ:
                this->stack_empurrar_16_bits(this->pc);
                this->stack_empurrar(this->get_estado() | 0x10);
                this->pc = this->memoria->ler_16_bits(0xFFFE);
                this->i = 1;
                this->ciclos += 7;
                break;

            default: break;
        }
        this->interrupcao = Interrupcao::NENHUMA;

        byte opcode = this->memoria->ler(this->pc);        
        
        // lançar erro se a instrução não existir na tabela
        if (!this->instrucoes.at(opcode).has_value())
        {
            stringstream erro_ss;
            erro_ss << "Opcode não reconhecido: ";
            erro_ss << "$" << std::hex << opcode;
            
            throw runtime_error(erro_ss.str());
        }

        auto instrucao = this->instrucoes.at(opcode).value();
        this->executar(&instrucao);
        
        this->ciclos += instrucao.ciclos;
        if (this->is_pag_alterada) 
        {
            this->ciclos += instrucao.ciclos_pag_alt;
        }
        
        return this->ciclos - ciclos;
    }

    void Cpu::executar(Instrucao* instrucao)
    {
        auto endereco = instrucao->buscar_endereco(this);
        
        this->pc += instrucao->bytes;
        instrucao->implementacao(this, instrucao->modo, endereco);
    }

    void Cpu::resetar()
    {
        this->pc = this->memoria->ler_16_bits(0xFFFC);
        this->sp = 0xFD;
        this->set_estado(0x24);
    }

    void Cpu::branch_somar_ciclos(uint16 endereco)
    {
        // somar 1 se os 2 endereços forem da mesma pagina,
        // somar 2 se forem de paginas diferentes
        if (comparar_paginas(this->pc, endereco))
            this->ciclos += 1;
        else
            this->ciclos += 2;
    }

    byte Cpu::get_estado()
    {
        byte flags = 0;

        const byte c = this->c;
        const byte z = this->z << 1;
        const byte i = this->i << 2;
        const byte d = this->d << 3;
        const byte b = this->b << 4;
        const byte v = this->v << 6;
        const byte n = this->n << 7;
        // o bit na posiçao 5 sempre está ativo
        const byte bit_5 = 1 << 5;

        return flags | c | z | i | d | b | bit_5 | v | n;
    }

    void Cpu::set_estado(byte valor)
    {
        this->c = buscar_bit(valor, 0);
        this->z = buscar_bit(valor, 1);
        this->i = buscar_bit(valor, 2);
        this->d = buscar_bit(valor, 3);
        this->b = buscar_bit(valor, 4);
        this->v = buscar_bit(valor, 6);
        this->n = buscar_bit(valor, 7);
    }

    void Cpu::stack_empurrar(byte valor)
    {
        uint16 endereco = 0x0100 | this->sp;
        this->memoria->escrever(endereco, valor);

        this->sp -= 1;
    }

    void Cpu::stack_empurrar_16_bits(uint16 valor)
    {
        byte menor = valor & 0x00FF;
        byte maior = (valor & 0xFF00) >> 8;

        this->stack_empurrar(maior);
        this->stack_empurrar(menor);
    }

    byte Cpu::stack_puxar()
    {
        this->sp += 1;
        uint16 endereco = 0x0100 | this->sp;
        return this->memoria->ler(endereco);
    }

    uint16 Cpu::stack_puxar_16_bits()
    {
        byte menor = this->stack_puxar();
        byte maior = this->stack_puxar();

        return (maior << 8) | menor;
    }

    void Cpu::esperar_adicionar(uint16 esperar)
    {
        this->esperar += esperar;
    }

    void Cpu::set_z(byte valor)
    {
        // checa se um valor é '0'
        if (valor == 0)
            this->z = true;
        else
            this->z = false;
    }

    void Cpu::set_n(byte valor)
    {
        // o valor é negativo se o bit mais significativo não for '0'
        if ((valor & 0b10000000) != 0)
            this->n = true;
        else
            this->n = false;
    }
    
    uint32 Cpu::get_ciclos()
    {
        return this->ciclos;
    }

    uint16 Cpu::get_esperar()
    {
        return this->esperar;
    }

    optional<Instrucao> Cpu::get_instrucao(byte opcode)
    {
        return this->instrucoes.at(opcode);
    }

    string Cpu::instrucao_para_asm(byte opcode)
    {
        // lançar erro se a instrução não existir na tabela
        if (!this->instrucoes.at(opcode).has_value())
        {
            stringstream erro_ss;
            erro_ss << "Instrução não reconhecida: ";
            erro_ss << "$" << std::hex << opcode;
            
            throw runtime_error(erro_ss.str());
        }

        auto instrucao = this->instrucoes.at(opcode).value();

        switch (instrucao.modo)
        {
            case InstrucaoModo::ACM:
            {   
                stringstream ss;
                
                ss << instrucao.nome << " $" << std::uppercase << std::hex << static_cast<int>(this->a);

                return ss.str();
            }

            case InstrucaoModo::ABS:
            {
                stringstream ss;

                uint16 endereco = instrucao.buscar_endereco(this).value();
                ss << instrucao.nome << " $" << std::uppercase << std::hex << endereco;
            
                return ss.str();
            }

            case InstrucaoModo::ABS_X:
            {
                stringstream ss;

                uint16 endereco = this->memoria->ler_16_bits(this->pc + 1);
                ss << instrucao.nome << " $" << std::uppercase << std::hex << endereco << ", X";

                return ss.str();
            }

            case InstrucaoModo::ABS_Y:
            {
                stringstream ss;

                uint16 valor = this->memoria->ler_16_bits(this->pc + 1);
                ss << instrucao.nome << " $" << std::uppercase << std::hex << valor << ", Y";

                return ss.str();
            }

            case InstrucaoModo::IMED:
            {
                int valor = this->memoria->ler(this->pc + 1);

                stringstream ss;
                ss << instrucao.nome << " #$" << std::uppercase << std::hex << valor;

                return ss.str();
            }

            case InstrucaoModo::IMPL:
            {
                stringstream ss;
                ss << instrucao.nome;

                return ss.str();
            }

            case InstrucaoModo::IND:
            {
                stringstream ss;
                ss << instrucao.nome << " ($" << std::uppercase << std::hex << this->pc+1 << ")";

                return ss.str();
            }

            case InstrucaoModo::IND_X:
            {
                int valor = this->memoria->ler(this->pc + 1);

                stringstream ss;
                ss << instrucao.nome << " ($" << std::uppercase << std::hex << valor << ", X)";

                return ss.str();
            }

            case InstrucaoModo::IND_Y:
            {
                int valor = this->memoria->ler(this->pc + 1);

                stringstream ss;
                ss << instrucao.nome << " ($" << std::uppercase << std::hex << valor << "), Y";

                return ss.str();
            }

            case InstrucaoModo::REL:
            {
                int endereco = instrucao.buscar_endereco(this).value();

                stringstream ss;
                ss << instrucao.nome << " $" << std::uppercase << std::hex << endereco << "";

                return ss.str();
            }

            case InstrucaoModo::P_ZERO:
            {
                int valor = this->memoria->ler(this->pc + 1);

                stringstream ss;
                ss << instrucao.nome << " $" << std::uppercase << std::hex << valor;

                return ss.str();
            }

            case InstrucaoModo::P_ZERO_X:
            {
                int valor = this->memoria->ler(this->pc + 1);

                stringstream ss;
                ss << instrucao.nome << " $" << std::uppercase << std::hex << valor << ", X";

                return ss.str();
            }

            case InstrucaoModo::P_ZERO_Y:
            {
                int valor = this->memoria->ler(this->pc + 1);

                stringstream ss;
                ss << instrucao.nome << " $" << std::uppercase << std::hex << valor << ", Y";

                return ss.str();
            }

            default:
                return "???";
        }
    }
}
