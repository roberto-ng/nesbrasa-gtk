#include <nesbrasa/cpu.hpp>
#include <sstream>

namespace nesbrasa::nucleo
{
using namespace nesbrasa::tipos;
using std::array;
using std::optional;
using std::string;
using std::stringstream;
using std::runtime_error;

Cpu::Cpu(CpuBus* memoria):
        instrucoes(carregar_instrucoes()) {

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

uint Cpu::avancar() {

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

void Cpu::executar(Instrucao* instrucao) {

        auto endereco = this->buscar_endereco(instrucao->modo);
        
        this->pc += instrucao->bytes;
        this->executar_operacao(instrucao->operacao, instrucao->modo, endereco);
    
}

optional<uint16> Cpu::buscar_endereco(InstrucaoModo modo) {

        this->is_pag_alterada = false;

        switch (modo)
        {
            case InstrucaoModo::ACM:
            case InstrucaoModo::IMPL: return std::nullopt;
            case InstrucaoModo::IMED: return this->pc + 1;
            case InstrucaoModo::P_ZERO: return this->memoria->ler(this->pc + 1);
            case InstrucaoModo::P_ZERO_X: return (this->memoria->ler(this->pc + 1) + this->x) & 0xFF;
            case InstrucaoModo::P_ZERO_Y: return (this->memoria->ler(this->pc + 1) + this->y) & 0xFF;
            case InstrucaoModo::ABS: return this->memoria->ler_16_bits(this->pc + 1);
            case InstrucaoModo::ABS_X:
            {
                uint16 endereco = this->memoria->ler_16_bits(this->pc + 1) + this->x;
                this->is_pag_alterada = !comparar_paginas(endereco - this->x, endereco);
                return endereco;
            }
            case InstrucaoModo::ABS_Y:
            {
                uint16 endereco = this->memoria->ler_16_bits(this->pc + 1) + this->y;
                this->is_pag_alterada = !comparar_paginas(endereco - this->y, endereco);
                return endereco;
            }
            case InstrucaoModo::IND: return this->memoria->ler_16_bits_bug(this->memoria->ler_16_bits(this->pc + 1));
            case InstrucaoModo::IND_X: return this->memoria->ler_16_bits_bug((this->memoria->ler(this->pc + 1) + this->x) % 0x100);
            case InstrucaoModo::IND_Y:
            {
                uint16 endereco = this->memoria->ler_16_bits_bug(this->memoria->ler(this->pc + 1)) + this->y;
                this->is_pag_alterada = !comparar_paginas(endereco - this->y, endereco);
                return endereco;
            }
            case InstrucaoModo::REL:
            {
                uint16 valor = this->memoria->ler(this->pc + 1);
                return valor < 0x80 ? this->pc + 2 + valor : this->pc + 2 + valor - 0x100;
            }
        }

        return 0;
    
}

void Cpu::resetar() {

        this->pc = this->memoria->ler_16_bits(0xFFFC);
        this->sp = 0xFD;
        this->set_estado(0x24);
    
}

void Cpu::branch_somar_ciclos(uint16 endereco) {

        // somar 1 se os 2 endereços forem da mesma pagina,
        // somar 2 se forem de paginas diferentes
        if (comparar_paginas(this->pc, endereco))
            this->ciclos += 1;
        else
            this->ciclos += 2;
    
}

byte Cpu::get_estado() {

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

void Cpu::set_estado(byte valor) {

        this->c = buscar_bit(valor, 0);
        this->z = buscar_bit(valor, 1);
        this->i = buscar_bit(valor, 2);
        this->d = buscar_bit(valor, 3);
        this->b = buscar_bit(valor, 4);
        this->v = buscar_bit(valor, 6);
        this->n = buscar_bit(valor, 7);
    
}

void Cpu::stack_empurrar(byte valor) {

        uint16 endereco = 0x0100 | this->sp;
        this->memoria->escrever(endereco, valor);

        this->sp -= 1;
    
}

void Cpu::stack_empurrar_16_bits(uint16 valor) {

        byte menor = valor & 0x00FF;
        byte maior = (valor & 0xFF00) >> 8;

        this->stack_empurrar(maior);
        this->stack_empurrar(menor);
    
}

byte Cpu::stack_puxar() {

        this->sp += 1;
        uint16 endereco = 0x0100 | this->sp;
        return this->memoria->ler(endereco);
    
}

uint16 Cpu::stack_puxar_16_bits() {

        byte menor = this->stack_puxar();
        byte maior = this->stack_puxar();

        return (maior << 8) | menor;
    
}

void Cpu::esperar_adicionar(uint16 esperar) {

        this->esperar += esperar;
    
}

void Cpu::set_z(byte valor) {

        // checa se um valor é '0'
        if (valor == 0)
            this->z = true;
        else
            this->z = false;
    
}

void Cpu::set_n(byte valor) {

        // o valor é negativo se o bit mais significativo não for '0'
        if ((valor & 0b10000000) != 0)
            this->n = true;
        else
            this->n = false;
    
}

uint32 Cpu::get_ciclos() {

        return this->ciclos;
    
}

uint16 Cpu::get_esperar() {

        return this->esperar;
    
}

optional<Instrucao> Cpu::get_instrucao(byte opcode) {

        return this->instrucoes.at(opcode);
    
}

string Cpu::instrucao_para_asm(byte opcode) {

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

                uint16 endereco = this->buscar_endereco(instrucao.modo).value();
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
                int endereco = this->buscar_endereco(instrucao.modo).value();

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


using std::array;
using std::optional;
using std::nullopt;
using std::string;

namespace nesbrasa::nucleo
{
    /*!
    Instrução ADC
    A + M + C -> A, C
    */
    static void instrucao_adc(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        byte valor = cpu->memoria->ler(endereco.value());

        const byte a = cpu->a;
        const byte c = (cpu->c) ? 1 : 0;

        cpu->a = a + valor + c;

        // atualiza a flag c
        int32_t soma_total = (int)a + (int)valor + (int)c;
        if (soma_total > 0xFF)
            cpu->c = 1;
        else
            cpu->c = 0;

        // checa se houve um overflow/transbordamento e atualiza a flag v
        // solução baseada em: https://stackoverflow.com/a/16861251
        if ((~(a ^ valor) & (a ^ soma_total) & 0x80) != 0)
            cpu->v = 1;
        else
            cpu->v = 0;

        // atualiza as flags z e n
        cpu->set_n(cpu->a);
        cpu->set_z(cpu->a);
    }

    /*!
    Instrução AND
    A AND M -> A
    */
    static void instrucao_and(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        byte valor = cpu->memoria->ler(endereco.value());

        const byte a = cpu->a;
        const byte m = valor;

        cpu->a = a & m;

        // atualizar flags
        cpu->set_n(cpu->a);
        cpu->set_z(cpu->a);
    }

    /*!
    Instrução shift para a esquerda.
    Utiliza a memoria ou o acumulador
    */
    static void instrucao_asl(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        if (modo == InstrucaoModo::ACM)
        {
            // checa se a posição 7 do byte é '1' ou '0'
            cpu->c = buscar_bit(cpu->a, 7);

            cpu->a <<= 1;

            // atualizar flags
            cpu->set_n(cpu->a);
            cpu->set_z(cpu->a);
        }
        else
        {
            byte valor = cpu->memoria->ler(endereco.value());

            // checa se a posição 7 do byte é '1' ou '0'
            cpu->c = buscar_bit(valor, 7);

            valor <<= 1;

            cpu->memoria->escrever(endereco.value(), valor);

            // atualizar flags
            cpu->set_n(valor);
            cpu->set_z(valor);
        }
    }

    //! Pula para o endereço indicado se a flag 'c' não estiver ativa
    static void instrucao_bcc(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        if (cpu->c == false)
        {
            cpu->branch_somar_ciclos(endereco.value());
            cpu->pc = endereco.value();
        }
    }

    //! Pula para o endereço indicado se a flag 'c' estiver ativa
    static void instrucao_bcs(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        if (cpu->c == true)
        {
            cpu->branch_somar_ciclos(endereco.value());
            cpu->pc = endereco.value();
        }
    }

    //! Pula para o endereço indicado se a flag 'z' estiver ativa
    static void instrucao_beq(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        if (cpu->z == true)
        {
            cpu->branch_somar_ciclos(endereco.value());
            cpu->pc = endereco.value();
        }
    }

    /*! BIT
    Busca um byte na memoria e depois salva a posição 7 do byte em 'n'
    e a posição 6 do byte em 'v'.
    A flag 'z' tambem é alterada sendo calculada com 'a' AND valor
    */
    static void instrucao_bit(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        byte valor = cpu->memoria->ler(endereco.value());

        cpu->n = buscar_bit(valor, 7);
        cpu->v = buscar_bit(valor, 6);
        cpu->z = (valor & cpu->a) == 0;
    }

    //! Pula para o endereço indicado se a flag 'n' estiver ativa
    static void instrucao_bmi(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        if (cpu->n == true)
        {
            cpu->branch_somar_ciclos(endereco.value());
            cpu->pc = endereco.value();
        }
    }

    //! Pula para o endereço indicado se a flag 'z' não estiver ativa
    static void instrucao_bne(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        if (cpu->z == false)
        {
            cpu->branch_somar_ciclos(endereco.value());
            cpu->pc = endereco.value();
        }
    }

    //! Pula para o endereço indicado se a flag 'n' não estiver ativa
    static void instrucao_bpl(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        if (cpu->n == false)
        {
            cpu->branch_somar_ciclos(endereco.value());
            cpu->pc = endereco.value();
        }
    }

    //! Instrução BRK
    static void instrucao_brk(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        cpu->stack_empurrar_16_bits(cpu->pc);
        cpu->stack_empurrar(cpu->get_estado());

        cpu->b = 1;
        cpu->pc = cpu->memoria->ler_16_bits(0xFFFE);
    }

    //! Pula para o endereço indicado se a flag 'v' não estiver ativa
    static void instrucao_bvc(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        if (cpu->v == false)
        {
            cpu->branch_somar_ciclos(endereco.value());
            cpu->pc = endereco.value();
        }
    }

    //! Pula para o endereço indicado se a flag 'v' estiver ativa
    static void instrucao_bvs(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        if (cpu->v == true)
        {
            cpu->branch_somar_ciclos(endereco.value());
            cpu->pc = endereco.value();
        }
    }

    //! Limpa a flag 'c'
    static void instrucao_clc(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        cpu->c = false;
    }

    //! Limpa a flag 'd'
    static void instrucao_cld(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        cpu->d = false;
    }

    //! Limpa a flag 'i'
    static void instrucao_cli(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        cpu->i = false;
    }

    //! Limpa a flag 'v'
    static void instrucao_clv(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        cpu->v = false;
    }

    //! Compara o acumulador com um valor
    static void instrucao_cmp(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        byte valor = cpu->memoria->ler(endereco.value());

        if (cpu->a >= valor)
            cpu->c = true;
        else
            cpu->c = false;

        byte resultado = cpu->a - valor;

        // atualizar flags
        cpu->set_n(resultado);
        cpu->set_z(resultado);
    }

    //! Compara o indice X com um valor
    static void instrucao_cpx(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        byte valor = cpu->memoria->ler(endereco.value());

        if (cpu->x >= valor)
            cpu->c = true;
        else
            cpu->c = false;

        byte resultado = cpu->x - valor;

        // atualizar flags
        cpu->set_n(resultado);
        cpu->set_z(resultado);
    }

    //! Compara o indice Y com um valor
    static void instrucao_cpy(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        byte valor = cpu->memoria->ler(endereco.value());

        if (cpu->y >= valor)
            cpu->c = true;
        else
            cpu->c = false;

        byte resultado = cpu->y - valor;
        cpu->set_n(resultado);
        cpu->set_z (resultado);
    }

    //! Diminui um valor na memoria por 1
    static void instrucao_dec(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        byte valor = cpu->memoria->ler(endereco.value());

        valor -= 1;

        // atualizar o valor na memoria
        cpu->memoria->escrever(endereco.value(), valor);

        // atualizar flags
        cpu->set_n(valor);
        cpu->set_z(valor);
    }

    //! Diminui o valor do indice X por 1
    static void instrucao_dex(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        cpu->x -= 1;

        // atualizar flags
        cpu->set_n(cpu->x);
        cpu->set_z(cpu->x);
    }

    //! Diminui o valor do indice Y por 1
    static void instrucao_dey(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        cpu->y -= 1;

        // atualizar flags
        cpu->set_n(cpu->y);
        cpu->set_z(cpu->y);
    }

    //! OR exclusivo de um valor na memoria com o acumulador
    static void instrucao_eor(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        byte valor = cpu->memoria->ler(endereco.value());

        cpu->a = cpu->a ^ valor;

        //atualizar flags
        cpu->set_n(cpu->a);
        cpu->set_z(cpu->a);
    }

    //! Incrementa um valor na memoria por 1
    static void instrucao_inc(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        byte valor = cpu->memoria->ler(endereco.value());

        valor += 1;

        // atualizar o valor na memoria
        cpu->memoria->escrever(endereco.value(), valor);

        // atualizar flags
        cpu->set_n(valor);
        cpu->set_z(valor);
    }

    //! Incrementa o valor do indice X por 1
    static void instrucao_inx(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        cpu->x += 1;

        // atualizar flags
        cpu->set_n(cpu->x);
        cpu->set_z(cpu->x);
    }

    //! Incrementa o valor do indice Y por 1
    static void instrucao_iny(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        cpu->y += 1;

        // atualizar flags
        cpu->set_n(cpu->y);
        cpu->set_z(cpu->y);
    }

    //! Pula o programa para o endereço indicado
    static void instrucao_jmp(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        // muda o endereço
        cpu->pc = endereco.value();
    }

    //! Chama uma função/subrotina
    static void instrucao_jsr(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        // Salva o endereço da próxima instrução subtraído por 1 na stack.
        // O endereço guardado vai ser usado para retornar da função quando
        // o opcode 'rts' for usado
        cpu->stack_empurrar_16_bits(cpu->pc - 1);

        // muda o endereço atual do programa para o da função indicada
        cpu->pc = endereco.value();
    }

    //! Carrega um valor da memoria no acumulador
    static void instrucao_lda(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        cpu->a = cpu->memoria->ler(endereco.value());
        //std::cout << "A: " << std::bitset<8>(cpu->a) << "\n";

        // atualizar flags
        cpu->set_n(cpu->a);
        cpu->set_z(cpu->a);
    }


    //! Carrega um valor da memoria no indice X
    static void instrucao_ldx(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        cpu->x = cpu->memoria->ler(endereco.value());

        // atualizar flags
        cpu->set_n(cpu->x);
        cpu->set_z(cpu->x);
    }

    //! Carrega um valor da memoria no acumulador
    static void instrucao_ldy(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        cpu->y = cpu->memoria->ler(endereco.value());

        // atualizar flags
        cpu->set_n(cpu->y);
        cpu->set_z(cpu->y);
    }

    /*!
    Instrução shift para a direita.
    Utiliza a memoria ou o acumulador
    */
    static void instrucao_lsr(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        if (modo == InstrucaoModo::ACM)
        {
            // checa se a posição 0 do byte é '1' ou '0'
            cpu->c = buscar_bit(cpu->a, 0);

            cpu->a >>= 1;

            // atualizar flags
            cpu->set_n(cpu->a);
            cpu->set_z(cpu->a);
        }
        else
        {
            byte valor = cpu->memoria->ler(endereco.value());

            // checa se a posição 0 do byte é '1' ou '0'
            cpu->c = buscar_bit(valor, 0);

            valor >>= 1;

            cpu->memoria->escrever(endereco.value(), valor);

            // atualizar flags
            cpu->set_n(valor);
            cpu->set_z(valor);
        }
    }

    //! Não fazer nada
    static void instrucao_nop(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
    }

    //! Operanção OR entre um valor na memoria e o A
    static void instrucao_ora(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        byte valor = cpu->memoria->ler(endereco.value());

        cpu->a = cpu->a | valor;

        //atualizar flags
        cpu->set_n(cpu->a);
        cpu->set_z(cpu->a);
    }

    //! Empurra o valor do acumulador na stack
    static void instrucao_pha(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        cpu->stack_empurrar(cpu->a);
    }

    //! Empurra o valor do estado do processador na stack
    static void instrucao_php(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        const byte estado = cpu->get_estado();
        cpu->stack_empurrar(estado);
    }

    //! Puxa um valor da stack e salva esse valor no acumulador
    static void instrucao_pla(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        cpu->a = cpu->stack_puxar();

        // atualizar flags
        cpu->set_n(cpu->a);
        cpu->set_z(cpu->a);
    }

    //! Puxa um valor da stack e salva esse valor no estado do processador
    static void instrucao_plp(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        const byte estado = cpu->stack_puxar();
        cpu->set_estado(estado);
    }

    //! Gira um valor pra a esquerda
    static void instrucao_rol(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        if (modo == InstrucaoModo::ACM)
        {
            bool carregar = cpu->c;
            cpu->c = buscar_bit(cpu->a, 7);
            cpu->a <<= 1;
            cpu->a = cpu->a | ((carregar) ? 1 : 0);

            // atualizar flags
            cpu->set_n(cpu->a);
            cpu->set_z(cpu->a);
        }
        else
        {
            byte valor = cpu->memoria->ler(endereco.value());

            bool carregar = cpu->c;
            cpu->c = buscar_bit(valor, 7);
            valor <<= 1;
            valor = valor | ((carregar) ? 1 : 0);

            // atualizar o valor na memoria
            cpu->memoria->escrever(endereco.value(), valor);

            // atualizar flags
            cpu->set_n(cpu->a);
            cpu->set_z(cpu->a);
        }
    }

    //! Gira um valor pra a direita
    static void instrucao_ror(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        if (modo == InstrucaoModo::ACM)
        {
            bool carregar = cpu->c;
            cpu->c = buscar_bit(cpu->a, 0);
            cpu->a >>= 1;
            cpu->a = cpu->a | ((carregar) ? 0b10000000 : 0);

            // atualizar flags
            cpu->set_n(cpu->a);
            cpu->set_z(cpu->a);
        }
        else
        {
            byte valor = cpu->memoria->ler(endereco.value());

            bool carregar = cpu->c;
            cpu->c = buscar_bit(valor, 0);
            valor >>= 1;
            valor = valor | ((carregar) ? 0b10000000 : 0);

            // atualizar o valor na memoria
            cpu->memoria->escrever(endereco.value(), valor);

            // atualizar flags
            cpu->set_n(cpu->a);
            cpu->set_z(cpu->a);
        }
    }

    //! Retorna de uma interupção
    static void instrucao_rti(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        const byte estado = cpu->stack_puxar();
        cpu->set_estado(estado);

        cpu->pc = cpu->stack_puxar_16_bits();
    }

    //! Retorna de uma função/sub-rotina
    static void instrucao_rts(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        cpu->pc = cpu->stack_puxar_16_bits() + 1;
    }

    //! Subtrai um valor da memoria usando o acumulador
    static void instrucao_sbc(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        byte valor = cpu->memoria->ler(endereco.value());

        const byte a = cpu->a;
        const byte c = (!cpu->c) ? 1 : 0;

        cpu->a = a - valor - c;

        // atualiza a flag c
        int32_t subtracao_total = (int)a - (int)valor - (int)c;
        if (subtracao_total >= 0)
            cpu->c = 1;
        else
            cpu->c = 0;

        // checa se houve um overflow/transbordamento e atualiza a flag v
        if (((a ^ valor) & (a ^ subtracao_total) & 0x80) != 0)
            cpu->v = 1;
        else
            cpu->v = 0;

        // atualiza as flags z e n
        cpu->set_n(cpu->a);
        cpu->set_z(cpu->a);
    }

    //! Ativa a flag 'c'
    static void instrucao_sec(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        cpu->c = true;
    }

    //! Ativa a flag 'd'
    static void instrucao_sed(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        cpu->d = true;
    }

    //! Ativa a flag 'i'
    static void instrucao_sei(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        cpu->i = true;
    }

    //! Guarda o valor do acumulador na memoria
    static void instrucao_sta(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        cpu->memoria->escrever(endereco.value(), cpu->a);
    }

    //! Guarda o valor do registrador 'x' na memoria
    static void instrucao_stx(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        cpu->memoria->escrever(endereco.value(), cpu->x);
    }

    //! Guarda o valor do registrador 'y' na memoria
    static void instrucao_sty(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        cpu->memoria->escrever(endereco.value(), cpu->y);
    }

    //! Atribui o valor do acumulador ao registrador 'x'
    static void instrucao_tax(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        cpu->x = cpu->a;

        // atualizar flags
        cpu->set_n(cpu->x);
        cpu->set_z(cpu->x);
    }

    //! Atribui o valor do acumulador ao registrador 'y'
    static void instrucao_tay(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        cpu->y = cpu->a;

        // atualizar flags
        cpu->set_n(cpu->y);
        cpu->set_z(cpu->y);
    }

    //! Atribui o valor do ponteiro da stack ao registrador 'x'
    static void instrucao_tsx(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        cpu->x = cpu->sp;

        // atualizar flags
        cpu->set_n(cpu->x);
        cpu->set_z(cpu->x);
    }

    //! Atribui o valor do registrador 'x' ao acumulador
    static void instrucao_txa(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        cpu->a = cpu->x;

        // atualizar flags
        cpu->set_n(cpu->a);
        cpu->set_z(cpu->a);
    }

    //! Atribui o valor do registrador 'x' ao ponteiro da stack
    static void instrucao_txs(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        cpu->sp = cpu->x;
    }

    //! Atribui o valor do registrador 'y' ao acumulador
    static void instrucao_tya(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        cpu->a = cpu->y;

        // atualizar flags
        cpu->set_n(cpu->a);
        cpu->set_z(cpu->a);
    }

    //! Instrução não-oficial *DOP - nenhuma operação
    static void instrucao_dop(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
    }

    //! Instrução não-oficial *TOP - nenhuma operação
    static void instrucao_top(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
    }

    //! Instrução não-oficial *LAX - Transfere um valor da memória para A e X
    static void instrucao_lax(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        byte valor = cpu->memoria->ler(endereco.value());

        cpu->a = valor;
        cpu->x = valor;

        cpu->set_n(valor);
        cpu->set_z(valor);
    }

    //! Instrução não-oficial *SAX - Faz a operação AND entre o A e o X e guarda o resultado na memória
    static void instrucao_sax(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        byte valor = cpu->a & cpu->x;

        cpu->memoria->escrever(endereco.value(), valor);
    }

    //! Instrução não-oficial *DCP - Subtrai um valor da memória e compara o resultado com A
    static void instrucao_dcp(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        byte valor = cpu->memoria->ler(endereco.value());
        byte resultado = valor - 1;

        cpu->memoria->escrever(endereco.value(), resultado);

        // compara o resultado com A
        byte comparacao = cpu->a - resultado;

        if (cpu->a >= comparacao)
            cpu->c = true;
        else
            cpu->c = false;

        // atualizar flags
        cpu->set_n(comparacao);
        cpu->set_z(comparacao);
    }

    //! Instrução não-oficial *ISB - Incrementa um valor na memória, depois subtrai este valor por A
    static void instrucao_isb(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        byte valor = cpu->memoria->ler(endereco.value());
        byte resultado = valor + 1;

        cpu->memoria->escrever(endereco.value(), resultado);

        const byte a = cpu->a;
        const byte c = (!cpu->c) ? 1 : 0;

        cpu->a = a - resultado - c;

        // atualiza a flag c
        int subtracao_total = (int)a - (int)resultado - (int)c;
        if (subtracao_total >= 0)
            cpu->c = 1;
        else
            cpu->c = 0;

        // checa se houve um overflow/transbordamento e atualiza a flag v
        if (((a ^ resultado) & (a ^ subtracao_total) & 0x80) != 0)
            cpu->v = 1;
        else
            cpu->v = 0;

        // atualiza as flags z e n
        cpu->set_n(cpu->a);
        cpu->set_z(cpu->a);
    }

    /*! 
    Instrução não-oficial *SLO: 
    Realiza um shift para a esquerda em um valor,e depois a operação OR entre A e o valor
    */
    static void instrucao_slo(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        byte valor = cpu->memoria->ler(endereco.value());

        // checa se a posição 7 do byte é '1' ou '0'
        cpu->c = buscar_bit(valor, 7);

        valor <<= 1;

        cpu->memoria->escrever(endereco.value(), valor);

        cpu->a = cpu->a | valor;

        //atualizar flags
        cpu->set_n(cpu->a);
        cpu->set_z(cpu->a);
    }

    /*! 
    Instrução não-oficial *RLA: 
    Gira um valor na memória para a esquerda, e depois realiza a operação AND entre A e o valor
    */
    static void instrucao_rla(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        byte valor = cpu->memoria->ler(endereco.value());

        bool carregar = cpu->c;
        cpu->c = buscar_bit(valor, 7);

        valor <<= 1;
        valor = valor | ((carregar) ? 1 : 0);

        // atualizar o valor na memoria
        cpu->memoria->escrever(endereco.value(), valor);

        cpu->a = cpu->a & valor;

        // atualizar flags
        cpu->set_n(cpu->a);
        cpu->set_z(cpu->a);
    }

    /*! 
    Instrução não-oficial *SRE: 
    Realiza um shift para a direita em um valor, e depois a operação EOR entre A e o valor
    */
    static void instrucao_sre(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        byte valor = cpu->memoria->ler(endereco.value());

        cpu->c = buscar_bit(valor, 0);

        valor >>= 1;

        // atualizar o valor na memoria
        cpu->memoria->escrever(endereco.value(), valor);

        cpu->a = cpu->a ^ valor; 

        // atualizar flags
        cpu->set_n(cpu->a);
        cpu->set_z(cpu->a);
    }

    /*! 
    Instrução não-oficial *RRA: 
    Gira um valor na memória para a direita, e depois soma o valor com A e C
    */
    static void instrucao_rra(Cpu* cpu, InstrucaoModo modo, optional<uint16> endereco)
    {
        byte valor = cpu->memoria->ler(endereco.value());

        bool carregar = cpu->c;
        cpu->c = buscar_bit(valor, 0);

        valor >>= 1;
        valor = valor | ((carregar) ? 0b10000000 : 0);

        // atualizar o valor na memoria
        cpu->memoria->escrever(endereco.value(), valor);

        const byte a = cpu->a;
        const byte c = (cpu->c) ? 1 : 0;

        cpu->a = a + valor + c;

        // atualiza a flag c
        int soma_total = (int)a + (int)valor + (int)c;
        if (soma_total > 0xFF)
            cpu->c = 1;
        else
            cpu->c = 0;

        // checa se houve um overflow/transbordamento e atualiza a flag v
        // solução baseada em: https://stackoverflow.com/a/16861251
        if ((~(a ^ valor) & (a ^ soma_total) & 0x80) != 0)
            cpu->v = 1;
        else
            cpu->v = 0;

        // atualiza as flags z e n
        cpu->set_n(cpu->a);
        cpu->set_z(cpu->a);
    }

    inline array< optional<Instrucao>, 256 > carregar_instrucoes()
    {
        // cria um array que será usado como uma tabela de instruções
        array< optional<Instrucao>, 256 > instrucoes;
        for (auto& instrucao : instrucoes)
        {
            instrucao = nullopt;
        }

        // modos da instrução ADC
        instrucoes.at(0x69) = Instrucao("ADC", 2, 2, 0, InstrucaoModo::IMED, InstrucaoOperacao::ADC);
        instrucoes.at(0x65) = Instrucao("ADC", 2, 3, 0, InstrucaoModo::P_ZERO, InstrucaoOperacao::ADC);
        instrucoes.at(0x75) = Instrucao("ADC", 2, 4, 0, InstrucaoModo::P_ZERO_X, InstrucaoOperacao::ADC);
        instrucoes.at(0x6D) = Instrucao("ADC", 3, 4, 0, InstrucaoModo::ABS, InstrucaoOperacao::ADC);
        instrucoes.at(0x7D) = Instrucao("ADC", 3, 4, 1, InstrucaoModo::ABS_X, InstrucaoOperacao::ADC);
        instrucoes.at(0x79) = Instrucao("ADC", 3, 4, 1, InstrucaoModo::ABS_Y, InstrucaoOperacao::ADC);
        instrucoes.at(0x61) = Instrucao("ADC", 2, 6, 0, InstrucaoModo::IND_X, InstrucaoOperacao::ADC);
        instrucoes.at(0x71) = Instrucao("ADC", 2, 5, 1, InstrucaoModo::IND_Y, InstrucaoOperacao::ADC);

        // modos da instrução AND
        instrucoes.at(0x29) = Instrucao("AND", 2, 2, 0, InstrucaoModo::IMED, InstrucaoOperacao::AND);
        instrucoes.at(0x25) = Instrucao("AND", 2, 3, 0, InstrucaoModo::P_ZERO, InstrucaoOperacao::AND);
        instrucoes.at(0x35) = Instrucao("AND", 2, 4, 0, InstrucaoModo::P_ZERO_X, InstrucaoOperacao::AND);
        instrucoes.at(0x2D) = Instrucao("AND", 3, 4, 0, InstrucaoModo::ABS, InstrucaoOperacao::AND);
        instrucoes.at(0x3D) = Instrucao("AND", 3, 4, 1, InstrucaoModo::ABS_X, InstrucaoOperacao::AND);
        instrucoes.at(0x39) = Instrucao("AND", 3, 4, 1, InstrucaoModo::ABS_Y, InstrucaoOperacao::AND);
        instrucoes.at(0x21) = Instrucao("AND", 2, 6, 0, InstrucaoModo::IND_X, InstrucaoOperacao::AND);
        instrucoes.at(0x31) = Instrucao("AND", 2, 5, 1, InstrucaoModo::IND_Y, InstrucaoOperacao::AND);

        // modos da instrução ASL
        instrucoes.at(0x0A) = Instrucao("ASL", 1, 2, 0, InstrucaoModo::ACM, InstrucaoOperacao::ASL);
        instrucoes.at(0x06) = Instrucao("ASL", 2, 5, 0, InstrucaoModo::P_ZERO, InstrucaoOperacao::ASL);
        instrucoes.at(0x16) = Instrucao("ASL", 2, 6, 0, InstrucaoModo::P_ZERO_X, InstrucaoOperacao::ASL);
        instrucoes.at(0x0E) = Instrucao("ASL", 3, 6, 0, InstrucaoModo::ABS, InstrucaoOperacao::ASL);
        instrucoes.at(0x1E) = Instrucao("ASL", 3, 7, 0, InstrucaoModo::ABS_X, InstrucaoOperacao::ASL);

        // modos da instrução BCC
        instrucoes.at(0x90) = Instrucao("BCC", 2, 2, 0, InstrucaoModo::REL, InstrucaoOperacao::BCC);

        // modos da instrução BCS
        instrucoes.at(0xB0) = Instrucao("BCS", 2, 2, 0, InstrucaoModo::REL, InstrucaoOperacao::BCS);

        // modos da instrução BEQ
        instrucoes.at(0xF0) = Instrucao("BEQ", 2, 2, 0, InstrucaoModo::REL, InstrucaoOperacao::BEQ);

        // modos da instrução BIT
        instrucoes.at(0x24) = Instrucao("BIT", 2, 3, 0, InstrucaoModo::P_ZERO, InstrucaoOperacao::BIT);
        instrucoes.at(0x2C) = Instrucao("BIT", 3, 4, 0, InstrucaoModo::ABS, InstrucaoOperacao::BIT);

        // modos da instrução BMI
        instrucoes.at(0x30) = Instrucao("BMI", 2, 2, 0, InstrucaoModo::REL, InstrucaoOperacao::BMI);

        // modos da instrução BNE
        instrucoes.at(0xD0) = Instrucao("BNE", 2, 2, 0, InstrucaoModo::REL, InstrucaoOperacao::BNE);

        // modos da instrução BPL
        instrucoes.at(0x10) = Instrucao("BPL", 2, 2, 0, InstrucaoModo::REL, InstrucaoOperacao::BPL);

        // modos da instrução BRK
        instrucoes.at(0x00) = Instrucao("BRK", 1, 7, 0, InstrucaoModo::IMPL, InstrucaoOperacao::BRK);

        // modos da instrução BVC
        instrucoes.at(0x50) = Instrucao("BVC", 2, 2, 0, InstrucaoModo::REL, InstrucaoOperacao::BVC);

        // modos da instrução BVS
        instrucoes.at(0x70) = Instrucao("BVS", 2, 2, 0, InstrucaoModo::REL, InstrucaoOperacao::BVS);

        // modos da instrução CLC
        instrucoes.at(0x18) = Instrucao("CLC", 1, 2, 0, InstrucaoModo::IMPL, InstrucaoOperacao::CLC);

        // modos da instrução CLD
        instrucoes.at(0xD8) = Instrucao("CLD", 1, 2, 0, InstrucaoModo::IMPL, InstrucaoOperacao::CLD);

        // modos da instrução CLI
        instrucoes.at(0x58) = Instrucao("CLI", 1, 2, 0, InstrucaoModo::IMPL, InstrucaoOperacao::CLI);

        // modos da instrução CLV
        instrucoes.at(0xB8) = Instrucao("CLV", 1, 2, 0, InstrucaoModo::IMPL, InstrucaoOperacao::CLV);

        // modos da instrução CMP
        instrucoes.at(0xC9) = Instrucao("CMP", 2, 2, 0, InstrucaoModo::IMED, InstrucaoOperacao::CMP);
        instrucoes.at(0xC5) = Instrucao("CMP", 2, 3, 0, InstrucaoModo::P_ZERO, InstrucaoOperacao::CMP);
        instrucoes.at(0xD5) = Instrucao("CMP", 2, 4, 0, InstrucaoModo::P_ZERO_X, InstrucaoOperacao::CMP);
        instrucoes.at(0xCD) = Instrucao("CMP", 3, 4, 0, InstrucaoModo::ABS, InstrucaoOperacao::CMP);
        instrucoes.at(0xDD) = Instrucao("CMP", 3, 4, 1, InstrucaoModo::ABS_X, InstrucaoOperacao::CMP);
        instrucoes.at(0xD9) = Instrucao("CMP", 3, 4, 1, InstrucaoModo::ABS_Y, InstrucaoOperacao::CMP);
        instrucoes.at(0xC1) = Instrucao("CMP", 2, 6, 0, InstrucaoModo::IND_X, InstrucaoOperacao::CMP);
        instrucoes.at(0xD1) = Instrucao("CMP", 2, 5, 1, InstrucaoModo::IND_Y, InstrucaoOperacao::CMP);

        // modos da instrução CPX
        instrucoes.at(0xE0) = Instrucao("CPX", 2, 2, 0, InstrucaoModo::IMED, InstrucaoOperacao::CPX);
        instrucoes.at(0xE4) = Instrucao("CPX", 2, 3, 0, InstrucaoModo::P_ZERO, InstrucaoOperacao::CPX);
        instrucoes.at(0xEC) = Instrucao("CPX", 3, 4, 0, InstrucaoModo::ABS, InstrucaoOperacao::CPX);

        // modos da instrução CPY
        instrucoes.at(0xC0) = Instrucao("CPY", 2, 2, 0, InstrucaoModo::IMED, InstrucaoOperacao::CPY);
        instrucoes.at(0xC4) = Instrucao("CPY", 2, 3, 0, InstrucaoModo::P_ZERO, InstrucaoOperacao::CPY);
        instrucoes.at(0xCC) = Instrucao("CPY", 3, 4, 0, InstrucaoModo::ABS, InstrucaoOperacao::CPY);

        // modos da instrução DEC
        instrucoes.at(0xC6) = Instrucao("DEC", 2, 5, 0, InstrucaoModo::P_ZERO, InstrucaoOperacao::DEC);
        instrucoes.at(0xD6) = Instrucao("DEC", 2, 6, 0, InstrucaoModo::P_ZERO_X, InstrucaoOperacao::DEC);
        instrucoes.at(0xCE) = Instrucao("DEC", 3, 6, 0, InstrucaoModo::ABS, InstrucaoOperacao::DEC);
        instrucoes.at(0xDE) = Instrucao("DEC", 3, 7, 0, InstrucaoModo::ABS_X, InstrucaoOperacao::DEC);

        // modos da instrução DEX
        instrucoes.at(0xCA) = Instrucao("DEX", 1, 2, 0, InstrucaoModo::IMPL, InstrucaoOperacao::DEX);

        // modos da instrução DEY
        instrucoes.at(0x88) = Instrucao("DEX", 1, 2, 0, InstrucaoModo::IMPL, InstrucaoOperacao::DEY);

        // modos da instrução EOR
        instrucoes.at(0x49) = Instrucao("EOR", 2, 2, 0, InstrucaoModo::IMED, InstrucaoOperacao::EOR);
        instrucoes.at(0x45) = Instrucao("EOR", 2, 3, 0, InstrucaoModo::P_ZERO, InstrucaoOperacao::EOR);
        instrucoes.at(0x55) = Instrucao("EOR", 2, 4, 0, InstrucaoModo::P_ZERO_X, InstrucaoOperacao::EOR);
        instrucoes.at(0x4D) = Instrucao("EOR", 3, 4, 0, InstrucaoModo::ABS, InstrucaoOperacao::EOR);
        instrucoes.at(0x5D) = Instrucao("EOR", 3, 4, 1, InstrucaoModo::ABS_X, InstrucaoOperacao::EOR);
        instrucoes.at(0x59) = Instrucao("EOR", 3, 4, 1, InstrucaoModo::ABS_Y, InstrucaoOperacao::EOR);
        instrucoes.at(0x41) = Instrucao("EOR", 2, 6, 0, InstrucaoModo::IND_X, InstrucaoOperacao::EOR);
        instrucoes.at(0x51) = Instrucao("EOR", 2, 5, 1, InstrucaoModo::IND_Y, InstrucaoOperacao::EOR);

        // modos da instrução INC
        instrucoes.at(0xE6) = Instrucao("INC", 2, 5, 0, InstrucaoModo::P_ZERO, InstrucaoOperacao::INC);
        instrucoes.at(0xF6) = Instrucao("INC", 2, 6, 0, InstrucaoModo::P_ZERO_X, InstrucaoOperacao::INC);
        instrucoes.at(0xEE) = Instrucao("INC", 3, 6, 0, InstrucaoModo::ABS, InstrucaoOperacao::INC);
        instrucoes.at(0xFE) = Instrucao("INC", 3, 7, 0, InstrucaoModo::ABS_X, InstrucaoOperacao::INC);

        // modos da instrução INX
        instrucoes.at(0xE8) = Instrucao("INX", 1, 2, 0, InstrucaoModo::IMPL, InstrucaoOperacao::INX);

        // modos da instrução INY
        instrucoes.at(0xC8) = Instrucao("INY", 1, 2, 0, InstrucaoModo::IMPL, InstrucaoOperacao::INY);

        // modos da instrução JMP
        instrucoes.at(0x4C) = Instrucao("JMP", 3, 3, 0, InstrucaoModo::ABS, InstrucaoOperacao::JMP);
        instrucoes.at(0x6C) = Instrucao("JMP", 3, 5, 0, InstrucaoModo::IND, InstrucaoOperacao::JMP);

        // modos da instrução JSR
        instrucoes.at(0x20) = Instrucao("JSR", 3, 6, 0, InstrucaoModo::ABS, InstrucaoOperacao::JSR);

        // modos da instrução LDA
        instrucoes.at(0xA9) = Instrucao("LDA", 2, 2, 0, InstrucaoModo::IMED, InstrucaoOperacao::LDA);
        instrucoes.at(0xA5) = Instrucao("LDA", 2, 3, 0, InstrucaoModo::P_ZERO, InstrucaoOperacao::LDA);
        instrucoes.at(0xB5) = Instrucao("LDA", 2, 4, 0, InstrucaoModo::P_ZERO_X, InstrucaoOperacao::LDA);
        instrucoes.at(0xAD) = Instrucao("LDA", 3, 4, 0, InstrucaoModo::ABS, InstrucaoOperacao::LDA);
        instrucoes.at(0xBD) = Instrucao("LDA", 3, 4, 1, InstrucaoModo::ABS_X, InstrucaoOperacao::LDA);
        instrucoes.at(0xB9) = Instrucao("LDA", 3, 4, 1, InstrucaoModo::ABS_Y, InstrucaoOperacao::LDA);
        instrucoes.at(0xA1) = Instrucao("LDA", 2, 6, 0, InstrucaoModo::IND_X, InstrucaoOperacao::LDA);
        instrucoes.at(0xB1) = Instrucao("LDA", 2, 5, 1, InstrucaoModo::IND_Y, InstrucaoOperacao::LDA);

        // modos da instrução LDX
        instrucoes.at(0xA2) = Instrucao("LDX", 2, 2, 0, InstrucaoModo::IMED, InstrucaoOperacao::LDX);
        instrucoes.at(0xA6) = Instrucao("LDX", 2, 3, 0, InstrucaoModo::P_ZERO, InstrucaoOperacao::LDX);
        instrucoes.at(0xB6) = Instrucao("LDX", 2, 4, 0, InstrucaoModo::P_ZERO_Y, InstrucaoOperacao::LDX);
        instrucoes.at(0xAE) = Instrucao("LDX", 3, 4, 0, InstrucaoModo::ABS, InstrucaoOperacao::LDX);
        instrucoes.at(0xBE) = Instrucao("LDX", 3, 4, 1, InstrucaoModo::ABS_Y, InstrucaoOperacao::LDX);

        // modos da instrução LDY
        instrucoes.at(0xA0) = Instrucao("LDY", 2, 2, 0, InstrucaoModo::IMED, InstrucaoOperacao::LDY);
        instrucoes.at(0xA4) = Instrucao("LDY", 2, 3, 0, InstrucaoModo::P_ZERO, InstrucaoOperacao::LDY);
        instrucoes.at(0xB4) = Instrucao("LDY", 2, 4, 0, InstrucaoModo::P_ZERO_X, InstrucaoOperacao::LDY);
        instrucoes.at(0xAC) = Instrucao("LDY", 3, 4, 0, InstrucaoModo::ABS, InstrucaoOperacao::LDY);
        instrucoes.at(0xBC) = Instrucao("LDY", 3, 4, 1, InstrucaoModo::ABS_X, InstrucaoOperacao::LDY);

        // modos da instrução LSR
        instrucoes.at(0x4A) = Instrucao("LSR", 1, 2, 0, InstrucaoModo::ACM, InstrucaoOperacao::LSR);
        instrucoes.at(0x46) = Instrucao("LSR", 2, 5, 0, InstrucaoModo::P_ZERO, InstrucaoOperacao::LSR);
        instrucoes.at(0x56) = Instrucao("LSR", 2, 6, 0, InstrucaoModo::P_ZERO_X, InstrucaoOperacao::LSR);
        instrucoes.at(0x4E) = Instrucao("LSR", 3, 6, 0, InstrucaoModo::ABS, InstrucaoOperacao::LSR);
        instrucoes.at(0x5E) = Instrucao("LSR", 3, 7, 0, InstrucaoModo::ABS_X, InstrucaoOperacao::LSR);

        // modos da instrução NOP
        instrucoes.at(0xEA) = Instrucao("NOP", 1, 2, 0, InstrucaoModo::IMPL, InstrucaoOperacao::NOP);
        // opcodes não-oficiais da instrução NOP
        instrucoes.at(0x1A) = Instrucao("*NOP", 1, 2, 0, InstrucaoModo::IMPL, InstrucaoOperacao::NOP);
        instrucoes.at(0x3A) = Instrucao("*NOP", 1, 2, 0, InstrucaoModo::IMPL, InstrucaoOperacao::NOP);
        instrucoes.at(0x5A) = Instrucao("*NOP", 1, 2, 0, InstrucaoModo::IMPL, InstrucaoOperacao::NOP);
        instrucoes.at(0x7A) = Instrucao("*NOP", 1, 2, 0, InstrucaoModo::IMPL, InstrucaoOperacao::NOP);
        instrucoes.at(0xDA) = Instrucao("*NOP", 1, 2, 0, InstrucaoModo::IMPL, InstrucaoOperacao::NOP);
        instrucoes.at(0xFA) = Instrucao("*NOP", 1, 2, 0, InstrucaoModo::IMPL, InstrucaoOperacao::NOP);

        // modos da instrução ORA
        instrucoes.at(0x09) = Instrucao("ORA", 2, 2, 0, InstrucaoModo::IMED, InstrucaoOperacao::ORA);
        instrucoes.at(0x05) = Instrucao("ORA", 2, 3, 0, InstrucaoModo::P_ZERO, InstrucaoOperacao::ORA);
        instrucoes.at(0x15) = Instrucao("ORA", 2, 4, 0, InstrucaoModo::P_ZERO_X, InstrucaoOperacao::ORA);
        instrucoes.at(0x0D) = Instrucao("ORA", 3, 4, 0, InstrucaoModo::ABS, InstrucaoOperacao::ORA);
        instrucoes.at(0x1D) = Instrucao("ORA", 3, 4, 1, InstrucaoModo::ABS_X, InstrucaoOperacao::ORA);
        instrucoes.at(0x19) = Instrucao("ORA", 3, 4, 1, InstrucaoModo::ABS_Y, InstrucaoOperacao::ORA);
        instrucoes.at(0x01) = Instrucao("ORA", 2, 6, 0, InstrucaoModo::IND_X, InstrucaoOperacao::ORA);
        instrucoes.at(0x11) = Instrucao("ORA", 2, 5, 1, InstrucaoModo::IND_Y, InstrucaoOperacao::ORA);

        // modos da instrução PHA
        instrucoes.at(0x48) = Instrucao("PHA", 1, 3, 0, InstrucaoModo::IMPL, InstrucaoOperacao::PHA);

        // modos da instrução PHP
        instrucoes.at(0x08) = Instrucao("PHP", 1, 3, 0, InstrucaoModo::IMPL, InstrucaoOperacao::PHP);

        // modos da instrução PLA
        instrucoes.at(0x68) = Instrucao("PLA", 1, 4, 0, InstrucaoModo::IMPL, InstrucaoOperacao::PLA);

        // modos da instrução PLP
        instrucoes.at(0x28) = Instrucao("PLP", 1, 4, 0, InstrucaoModo::IMPL, InstrucaoOperacao::PLP);

        // modos da instrução ROL
        instrucoes.at(0x2A) = Instrucao("ROL", 1, 2, 0, InstrucaoModo::ACM, InstrucaoOperacao::ROL);
        instrucoes.at(0x26) = Instrucao("ROL", 2, 5, 0, InstrucaoModo::P_ZERO, InstrucaoOperacao::ROL);
        instrucoes.at(0x36) = Instrucao("ROL", 2, 6, 0, InstrucaoModo::P_ZERO_X, InstrucaoOperacao::ROL);
        instrucoes.at(0x2E) = Instrucao("ROL", 3, 6, 0, InstrucaoModo::ABS, InstrucaoOperacao::ROL);
        instrucoes.at(0x3E) = Instrucao("ROL", 3, 7, 0, InstrucaoModo::ABS_X, InstrucaoOperacao::ROL);

        // modos da instrução ROR
        instrucoes.at(0x6A) = Instrucao("ROR", 1, 2, 0, InstrucaoModo::ACM, InstrucaoOperacao::ROR);
        instrucoes.at(0x66) = Instrucao("ROR", 2, 5, 0, InstrucaoModo::P_ZERO, InstrucaoOperacao::ROR);
        instrucoes.at(0x76) = Instrucao("ROR", 2, 6, 0, InstrucaoModo::P_ZERO_X, InstrucaoOperacao::ROR);
        instrucoes.at(0x6E) = Instrucao("ROR", 3, 6, 0, InstrucaoModo::ABS, InstrucaoOperacao::ROR);
        instrucoes.at(0x7E) = Instrucao("ROR", 3, 7, 0, InstrucaoModo::ABS_X, InstrucaoOperacao::ROR);

        // modos da instrução RTI
        instrucoes.at(0x40) = Instrucao("RTI", 1, 6, 0, InstrucaoModo::IMPL, InstrucaoOperacao::RTI);

        // modos da instrução RTS
        instrucoes.at(0x60) = Instrucao("RTS", 1, 6, 0, InstrucaoModo::IMPL, InstrucaoOperacao::RTS);

        // modos da instrução SBC
        instrucoes.at(0xE9) = Instrucao("SBC", 2, 2, 0, InstrucaoModo::IMED, InstrucaoOperacao::SBC);
        instrucoes.at(0xE5) = Instrucao("SBC", 2, 3, 0, InstrucaoModo::P_ZERO, InstrucaoOperacao::SBC);
        instrucoes.at(0xF5) = Instrucao("SBC", 2, 4, 0, InstrucaoModo::P_ZERO_X, InstrucaoOperacao::SBC);
        instrucoes.at(0xED) = Instrucao("SBC", 3, 4, 0, InstrucaoModo::ABS, InstrucaoOperacao::SBC);
        instrucoes.at(0xFD) = Instrucao("SBC", 3, 4, 1, InstrucaoModo::ABS_X, InstrucaoOperacao::SBC);
        instrucoes.at(0xF9) = Instrucao("SBC", 3, 4, 1, InstrucaoModo::ABS_Y, InstrucaoOperacao::SBC);
        instrucoes.at(0xE1) = Instrucao("SBC", 2, 6, 0, InstrucaoModo::IND_X, InstrucaoOperacao::SBC);
        instrucoes.at(0xF1) = Instrucao("SBC", 2, 5, 1, InstrucaoModo::IND_Y, InstrucaoOperacao::SBC);
        // opcode não-oficial da instrução SBC
        instrucoes.at(0xEB) = Instrucao("*SBC", 2, 2, 0, InstrucaoModo::IMED, InstrucaoOperacao::SBC);

        // modos da instrução SEC
        instrucoes.at(0x38) = Instrucao("SEC", 1, 2, 0, InstrucaoModo::IMPL, InstrucaoOperacao::SEC);

        // modos da instrução SED
        instrucoes.at(0xF8) = Instrucao("SED", 1, 2, 0, InstrucaoModo::IMPL, InstrucaoOperacao::SED);

        // modos da instrução SEI
        instrucoes.at(0x78) = Instrucao("SEI", 1, 2, 0, InstrucaoModo::IMPL, InstrucaoOperacao::SEI);

        // modos da instrução STA
        instrucoes.at(0x85) = Instrucao("STA", 2, 3, 0, InstrucaoModo::P_ZERO, InstrucaoOperacao::STA);
        instrucoes.at(0x95) = Instrucao("STA", 2, 4, 0, InstrucaoModo::P_ZERO_X, InstrucaoOperacao::STA);
        instrucoes.at(0x8D) = Instrucao("STA", 3, 4, 0, InstrucaoModo::ABS, InstrucaoOperacao::STA);
        instrucoes.at(0x9D) = Instrucao("STA", 3, 5, 0, InstrucaoModo::ABS_X, InstrucaoOperacao::STA);
        instrucoes.at(0x99) = Instrucao("STA", 3, 5, 0, InstrucaoModo::ABS_Y, InstrucaoOperacao::STA);
        instrucoes.at(0x81) = Instrucao("STA", 2, 6, 0, InstrucaoModo::IND_X, InstrucaoOperacao::STA);
        instrucoes.at(0x91) = Instrucao("STA", 2, 6, 0, InstrucaoModo::IND_Y, InstrucaoOperacao::STA);

        // modos da instrução STX
        instrucoes.at(0x86) = Instrucao("STX", 2, 3, 0, InstrucaoModo::P_ZERO, InstrucaoOperacao::STX);
        instrucoes.at(0x96) = Instrucao("STX", 2, 4, 0, InstrucaoModo::P_ZERO_Y, InstrucaoOperacao::STX);
        instrucoes.at(0x8E) = Instrucao("STX", 3, 4, 0, InstrucaoModo::ABS, InstrucaoOperacao::STX);

        // modos da instrução STY
        instrucoes.at(0x84) = Instrucao("STY", 2, 3, 0, InstrucaoModo::P_ZERO, InstrucaoOperacao::STY);
        instrucoes.at(0x94) = Instrucao("STY", 2, 4, 0, InstrucaoModo::P_ZERO_X, InstrucaoOperacao::STY);
        instrucoes.at(0x8C) = Instrucao("STY", 3, 4, 0, InstrucaoModo::ABS, InstrucaoOperacao::STY);

        // modos da instrução TAX
        instrucoes.at(0xAA) = Instrucao("TAX", 1, 2, 0, InstrucaoModo::IMPL, InstrucaoOperacao::TAX);

        // modos da instrução TAY
        instrucoes.at(0xA8) = Instrucao("TAY", 1, 2, 0, InstrucaoModo::IMPL, InstrucaoOperacao::TAY);

        // modos da instrução TSX
        instrucoes.at(0xBA) = Instrucao("TSX", 1, 2, 0, InstrucaoModo::IMPL, InstrucaoOperacao::TSX);

        // modos da instrução TXA
        instrucoes.at(0x8A) = Instrucao("TXA", 1, 2, 0, InstrucaoModo::IMPL, InstrucaoOperacao::TXA);

        // modos da instrução TXS
        instrucoes.at(0x9A) = Instrucao("TXS", 1, 2, 0, InstrucaoModo::IMPL, InstrucaoOperacao::TXS);

        // modos da instrução TYA
        instrucoes.at(0x98) = Instrucao("TYA", 1, 2, 0, InstrucaoModo::IMPL, InstrucaoOperacao::TYA);

        // modos da instrução não-oficial *DOP
        instrucoes.at(0x04) = Instrucao("*DOP", 2, 3, 0, InstrucaoModo::P_ZERO, InstrucaoOperacao::DOP);
        instrucoes.at(0x14) = Instrucao("*DOP", 2, 4, 0, InstrucaoModo::P_ZERO_X, InstrucaoOperacao::DOP);
        instrucoes.at(0x34) = Instrucao("*DOP", 2, 4, 0, InstrucaoModo::P_ZERO_X, InstrucaoOperacao::DOP);
        instrucoes.at(0x44) = Instrucao("*DOP", 2, 3, 0, InstrucaoModo::P_ZERO, InstrucaoOperacao::DOP);
        instrucoes.at(0x54) = Instrucao("*DOP", 2, 4, 0, InstrucaoModo::P_ZERO_X, InstrucaoOperacao::DOP);
        instrucoes.at(0x64) = Instrucao("*DOP", 2, 3, 0, InstrucaoModo::P_ZERO, InstrucaoOperacao::DOP);
        instrucoes.at(0x74) = Instrucao("*DOP", 2, 4, 0, InstrucaoModo::P_ZERO_X, InstrucaoOperacao::DOP);
        instrucoes.at(0x80) = Instrucao("*DOP", 2, 2, 0, InstrucaoModo::IMED, InstrucaoOperacao::DOP);
        instrucoes.at(0x82) = Instrucao("*DOP", 2, 2, 0, InstrucaoModo::IMED, InstrucaoOperacao::DOP);
        instrucoes.at(0x89) = Instrucao("*DOP", 2, 2, 0, InstrucaoModo::IMED, InstrucaoOperacao::DOP);
        instrucoes.at(0xC2) = Instrucao("*DOP", 2, 2, 0, InstrucaoModo::IMED, InstrucaoOperacao::DOP);
        instrucoes.at(0xD4) = Instrucao("*DOP", 2, 4, 0, InstrucaoModo::P_ZERO_X, InstrucaoOperacao::DOP);
        instrucoes.at(0xE2) = Instrucao("*DOP", 2, 2, 0, InstrucaoModo::IMED, InstrucaoOperacao::DOP);
        instrucoes.at(0xF4) = Instrucao("*DOP", 2, 4, 0, InstrucaoModo::P_ZERO_X, InstrucaoOperacao::DOP);

        // modos da instrução não-oficial *TOP
        instrucoes.at(0x0C) = Instrucao("*TOP", 3, 4, 0, InstrucaoModo::ABS, InstrucaoOperacao::TOP);
        instrucoes.at(0x1C) = Instrucao("*TOP", 3, 4, 1, InstrucaoModo::ABS_X, InstrucaoOperacao::TOP);
        instrucoes.at(0x3C) = Instrucao("*TOP", 3, 4, 1, InstrucaoModo::ABS_X, InstrucaoOperacao::TOP);
        instrucoes.at(0x5C) = Instrucao("*TOP", 3, 4, 1, InstrucaoModo::ABS_X, InstrucaoOperacao::TOP);
        instrucoes.at(0x7C) = Instrucao("*TOP", 3, 4, 1, InstrucaoModo::ABS_X, InstrucaoOperacao::TOP);
        instrucoes.at(0xDC) = Instrucao("*TOP", 3, 4, 1, InstrucaoModo::ABS_X, InstrucaoOperacao::TOP);
        instrucoes.at(0xFC) = Instrucao("*TOP", 3, 4, 1, InstrucaoModo::ABS_X, InstrucaoOperacao::TOP);

        // modos da instrução não-oficial *LAX
        instrucoes.at(0xA7) = Instrucao("*LAX", 2, 3, 0, InstrucaoModo::P_ZERO, InstrucaoOperacao::LAX);
        instrucoes.at(0xB7) = Instrucao("*LAX", 2, 4, 0, InstrucaoModo::P_ZERO_Y, InstrucaoOperacao::LAX);
        instrucoes.at(0xAF) = Instrucao("*LAX", 3, 4, 0, InstrucaoModo::ABS, InstrucaoOperacao::LAX);
        instrucoes.at(0xBF) = Instrucao("*LAX", 3, 4, 1, InstrucaoModo::ABS_Y, InstrucaoOperacao::LAX);
        instrucoes.at(0xA3) = Instrucao("*LAX", 2, 6, 0, InstrucaoModo::IND_X, InstrucaoOperacao::LAX);
        instrucoes.at(0xB3) = Instrucao("*LAX", 2, 5, 1, InstrucaoModo::IND_Y, InstrucaoOperacao::LAX);

        // modos da instrução não-oficial *SAX
        instrucoes.at(0x87) = Instrucao("*SAX", 2, 3, 0, InstrucaoModo::P_ZERO, InstrucaoOperacao::SAX);
        instrucoes.at(0x97) = Instrucao("*SAX", 2, 4, 0, InstrucaoModo::P_ZERO_Y, InstrucaoOperacao::SAX);
        instrucoes.at(0x83) = Instrucao("*SAX", 2, 6, 0, InstrucaoModo::IND_X, InstrucaoOperacao::SAX);
        instrucoes.at(0x8F) = Instrucao("*SAX", 3, 4, 0, InstrucaoModo::ABS, InstrucaoOperacao::SAX);

        // modos da instrução não-oficial *DCP
        instrucoes.at(0xC7) = Instrucao("*DCP", 2, 5, 0, InstrucaoModo::P_ZERO, InstrucaoOperacao::DCP);
        instrucoes.at(0xD7) = Instrucao("*DCP", 2, 6, 0, InstrucaoModo::P_ZERO_X, InstrucaoOperacao::DCP);
        instrucoes.at(0xCF) = Instrucao("*DCP", 3, 6, 0, InstrucaoModo::ABS, InstrucaoOperacao::DCP);
        instrucoes.at(0xDF) = Instrucao("*DCP", 3, 7, 0, InstrucaoModo::ABS_X, InstrucaoOperacao::DCP);
        instrucoes.at(0xDB) = Instrucao("*DCP", 3, 7, 0, InstrucaoModo::ABS_Y, InstrucaoOperacao::DCP);
        instrucoes.at(0xC3) = Instrucao("*DCP", 2, 8, 0, InstrucaoModo::IND_X, InstrucaoOperacao::DCP);
        instrucoes.at(0xD3) = Instrucao("*DCP", 2, 8, 0, InstrucaoModo::IND_Y, InstrucaoOperacao::DCP);

        // modos da instrução não-oficial *ISB
        instrucoes.at(0xE7) = Instrucao("*ISB", 2, 5, 0, InstrucaoModo::P_ZERO, InstrucaoOperacao::ISB);
        instrucoes.at(0xF7) = Instrucao("*ISB", 2, 6, 0, InstrucaoModo::P_ZERO_X, InstrucaoOperacao::ISB);
        instrucoes.at(0xEF) = Instrucao("*ISB", 3, 6, 0, InstrucaoModo::ABS, InstrucaoOperacao::ISB);
        instrucoes.at(0xFF) = Instrucao("*ISB", 3, 7, 0, InstrucaoModo::ABS_X, InstrucaoOperacao::ISB);
        instrucoes.at(0xFB) = Instrucao("*ISB", 3, 7, 0, InstrucaoModo::ABS_Y, InstrucaoOperacao::ISB);
        instrucoes.at(0xE3) = Instrucao("*ISB", 2, 8, 0, InstrucaoModo::IND_X, InstrucaoOperacao::ISB);
        instrucoes.at(0xF3) = Instrucao("*ISB", 2, 8, 0, InstrucaoModo::IND_Y, InstrucaoOperacao::ISB);

        // modos da instrução não-oficial *SLO
        instrucoes.at(0x07) = Instrucao("*SLO", 2, 5, 0, InstrucaoModo::P_ZERO, InstrucaoOperacao::SLO);
        instrucoes.at(0x17) = Instrucao("*SLO", 2, 6, 0, InstrucaoModo::P_ZERO_X, InstrucaoOperacao::SLO);
        instrucoes.at(0x0F) = Instrucao("*SLO", 3, 6, 0, InstrucaoModo::ABS, InstrucaoOperacao::SLO);
        instrucoes.at(0x1F) = Instrucao("*SLO", 3, 7, 0, InstrucaoModo::ABS_X, InstrucaoOperacao::SLO);
        instrucoes.at(0x1B) = Instrucao("*SLO", 3, 7, 0, InstrucaoModo::ABS_Y, InstrucaoOperacao::SLO);
        instrucoes.at(0x03) = Instrucao("*SLO", 2, 8, 0, InstrucaoModo::IND_X, InstrucaoOperacao::SLO);
        instrucoes.at(0x13) = Instrucao("*SLO", 2, 8, 0, InstrucaoModo::IND_Y, InstrucaoOperacao::SLO);

        // modos da instrução não-oficial *RLA
        instrucoes.at(0x27) = Instrucao("*RLA", 2, 5, 0, InstrucaoModo::P_ZERO, InstrucaoOperacao::RLA);
        instrucoes.at(0x37) = Instrucao("*RLA", 2, 6, 0, InstrucaoModo::P_ZERO_X, InstrucaoOperacao::RLA);
        instrucoes.at(0x2F) = Instrucao("*RLA", 3, 6, 0, InstrucaoModo::ABS, InstrucaoOperacao::RLA);
        instrucoes.at(0x3F) = Instrucao("*RLA", 3, 7, 0, InstrucaoModo::ABS_X, InstrucaoOperacao::RLA);
        instrucoes.at(0x3B) = Instrucao("*RLA", 3, 7, 0, InstrucaoModo::ABS_Y, InstrucaoOperacao::RLA);
        instrucoes.at(0x23) = Instrucao("*RLA", 2, 8, 0, InstrucaoModo::IND_X, InstrucaoOperacao::RLA);
        instrucoes.at(0x33) = Instrucao("*RLA", 2, 8, 0, InstrucaoModo::IND_Y, InstrucaoOperacao::RLA);

        // modos da instrução não-oficial *SRE
        instrucoes.at(0x47) = Instrucao("*SRE", 2, 5, 0, InstrucaoModo::P_ZERO, InstrucaoOperacao::SRE);
        instrucoes.at(0x57) = Instrucao("*SRE", 2, 6, 0, InstrucaoModo::P_ZERO_X, InstrucaoOperacao::SRE);
        instrucoes.at(0x4F) = Instrucao("*SRE", 3, 6, 0, InstrucaoModo::ABS, InstrucaoOperacao::SRE);
        instrucoes.at(0x5F) = Instrucao("*SRE", 3, 7, 0, InstrucaoModo::ABS_X, InstrucaoOperacao::SRE);
        instrucoes.at(0x5B) = Instrucao("*SRE", 3, 7, 0, InstrucaoModo::ABS_Y, InstrucaoOperacao::SRE);
        instrucoes.at(0x43) = Instrucao("*SRE", 2, 8, 0, InstrucaoModo::IND_X, InstrucaoOperacao::SRE);
        instrucoes.at(0x53) = Instrucao("*SRE", 2, 8, 0, InstrucaoModo::IND_Y, InstrucaoOperacao::SRE);

        // modos da instrução não-oficial *RRA
        instrucoes.at(0x67) = Instrucao("*RRA", 2, 5, 0, InstrucaoModo::P_ZERO, InstrucaoOperacao::RRA);
        instrucoes.at(0x77) = Instrucao("*RRA", 2, 6, 0, InstrucaoModo::P_ZERO_X, InstrucaoOperacao::RRA);
        instrucoes.at(0x6F) = Instrucao("*RRA", 3, 6, 0, InstrucaoModo::ABS, InstrucaoOperacao::RRA);
        instrucoes.at(0x7F) = Instrucao("*RRA", 3, 7, 0, InstrucaoModo::ABS_X, InstrucaoOperacao::RRA);
        instrucoes.at(0x7B) = Instrucao("*RRA", 3, 7, 0, InstrucaoModo::ABS_Y, InstrucaoOperacao::RRA);
        instrucoes.at(0x63) = Instrucao("*RRA", 2, 8, 0, InstrucaoModo::IND_X, InstrucaoOperacao::RRA);
        instrucoes.at(0x73) = Instrucao("*RRA", 2, 8, 0, InstrucaoModo::IND_Y, InstrucaoOperacao::RRA);

        return instrucoes;
    }
}

namespace nesbrasa::nucleo
{
    inline void Cpu::executar_operacao(InstrucaoOperacao operacao, InstrucaoModo modo,
                                optional<uint16> endereco)
    {
        switch (operacao)
        {
            case InstrucaoOperacao::ADC: instrucao_adc(this, modo, endereco); break;
            case InstrucaoOperacao::AND: instrucao_and(this, modo, endereco); break;
            case InstrucaoOperacao::ASL: instrucao_asl(this, modo, endereco); break;
            case InstrucaoOperacao::BCC: instrucao_bcc(this, modo, endereco); break;
            case InstrucaoOperacao::BCS: instrucao_bcs(this, modo, endereco); break;
            case InstrucaoOperacao::BEQ: instrucao_beq(this, modo, endereco); break;
            case InstrucaoOperacao::BIT: instrucao_bit(this, modo, endereco); break;
            case InstrucaoOperacao::BMI: instrucao_bmi(this, modo, endereco); break;
            case InstrucaoOperacao::BNE: instrucao_bne(this, modo, endereco); break;
            case InstrucaoOperacao::BPL: instrucao_bpl(this, modo, endereco); break;
            case InstrucaoOperacao::BRK: instrucao_brk(this, modo, endereco); break;
            case InstrucaoOperacao::BVC: instrucao_bvc(this, modo, endereco); break;
            case InstrucaoOperacao::BVS: instrucao_bvs(this, modo, endereco); break;
            case InstrucaoOperacao::CLC: instrucao_clc(this, modo, endereco); break;
            case InstrucaoOperacao::CLD: instrucao_cld(this, modo, endereco); break;
            case InstrucaoOperacao::CLI: instrucao_cli(this, modo, endereco); break;
            case InstrucaoOperacao::CLV: instrucao_clv(this, modo, endereco); break;
            case InstrucaoOperacao::CMP: instrucao_cmp(this, modo, endereco); break;
            case InstrucaoOperacao::CPX: instrucao_cpx(this, modo, endereco); break;
            case InstrucaoOperacao::CPY: instrucao_cpy(this, modo, endereco); break;
            case InstrucaoOperacao::DCP: instrucao_dcp(this, modo, endereco); break;
            case InstrucaoOperacao::DEC: instrucao_dec(this, modo, endereco); break;
            case InstrucaoOperacao::DEX: instrucao_dex(this, modo, endereco); break;
            case InstrucaoOperacao::DEY: instrucao_dey(this, modo, endereco); break;
            case InstrucaoOperacao::DOP: instrucao_dop(this, modo, endereco); break;
            case InstrucaoOperacao::EOR: instrucao_eor(this, modo, endereco); break;
            case InstrucaoOperacao::INC: instrucao_inc(this, modo, endereco); break;
            case InstrucaoOperacao::INX: instrucao_inx(this, modo, endereco); break;
            case InstrucaoOperacao::INY: instrucao_iny(this, modo, endereco); break;
            case InstrucaoOperacao::ISB: instrucao_isb(this, modo, endereco); break;
            case InstrucaoOperacao::JMP: instrucao_jmp(this, modo, endereco); break;
            case InstrucaoOperacao::JSR: instrucao_jsr(this, modo, endereco); break;
            case InstrucaoOperacao::LAX: instrucao_lax(this, modo, endereco); break;
            case InstrucaoOperacao::LDA: instrucao_lda(this, modo, endereco); break;
            case InstrucaoOperacao::LDX: instrucao_ldx(this, modo, endereco); break;
            case InstrucaoOperacao::LDY: instrucao_ldy(this, modo, endereco); break;
            case InstrucaoOperacao::LSR: instrucao_lsr(this, modo, endereco); break;
            case InstrucaoOperacao::NOP: instrucao_nop(this, modo, endereco); break;
            case InstrucaoOperacao::ORA: instrucao_ora(this, modo, endereco); break;
            case InstrucaoOperacao::PHA: instrucao_pha(this, modo, endereco); break;
            case InstrucaoOperacao::PHP: instrucao_php(this, modo, endereco); break;
            case InstrucaoOperacao::PLA: instrucao_pla(this, modo, endereco); break;
            case InstrucaoOperacao::PLP: instrucao_plp(this, modo, endereco); break;
            case InstrucaoOperacao::RLA: instrucao_rla(this, modo, endereco); break;
            case InstrucaoOperacao::ROL: instrucao_rol(this, modo, endereco); break;
            case InstrucaoOperacao::ROR: instrucao_ror(this, modo, endereco); break;
            case InstrucaoOperacao::RRA: instrucao_rra(this, modo, endereco); break;
            case InstrucaoOperacao::RTI: instrucao_rti(this, modo, endereco); break;
            case InstrucaoOperacao::RTS: instrucao_rts(this, modo, endereco); break;
            case InstrucaoOperacao::SAX: instrucao_sax(this, modo, endereco); break;
            case InstrucaoOperacao::SBC: instrucao_sbc(this, modo, endereco); break;
            case InstrucaoOperacao::SEC: instrucao_sec(this, modo, endereco); break;
            case InstrucaoOperacao::SED: instrucao_sed(this, modo, endereco); break;
            case InstrucaoOperacao::SEI: instrucao_sei(this, modo, endereco); break;
            case InstrucaoOperacao::SLO: instrucao_slo(this, modo, endereco); break;
            case InstrucaoOperacao::SRE: instrucao_sre(this, modo, endereco); break;
            case InstrucaoOperacao::STA: instrucao_sta(this, modo, endereco); break;
            case InstrucaoOperacao::STX: instrucao_stx(this, modo, endereco); break;
            case InstrucaoOperacao::STY: instrucao_sty(this, modo, endereco); break;
            case InstrucaoOperacao::TAX: instrucao_tax(this, modo, endereco); break;
            case InstrucaoOperacao::TAY: instrucao_tay(this, modo, endereco); break;
            case InstrucaoOperacao::TOP: instrucao_top(this, modo, endereco); break;
            case InstrucaoOperacao::TSX: instrucao_tsx(this, modo, endereco); break;
            case InstrucaoOperacao::TXA: instrucao_txa(this, modo, endereco); break;
            case InstrucaoOperacao::TXS: instrucao_txs(this, modo, endereco); break;
            case InstrucaoOperacao::TYA: instrucao_tya(this, modo, endereco); break;
        }
    }
}
