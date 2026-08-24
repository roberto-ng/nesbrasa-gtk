/* instrucao.cpp
 *
 * Copyright 2019 Roberto Nazareth <nazarethroberto97@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <memory>
#include <iostream>
#include <bitset>

#include "nesbrasa.hpp"
#include "cpu.hpp"
#include "instrucao.hpp"
#include "memoria.hpp"
#include "util.hpp"

namespace nesbrasa::nucleo
{
    Instrucao::Instrucao(
        string nome,
        byte bytes,
        int32 ciclos,
        int32 ciclos_pag_alt,
        InstrucaoModo modo,
        InstrucaoImplementacao implementacao
    )
    {
        this->nome = nome;
        this->bytes = bytes;
        this->ciclos = ciclos;
        this->ciclos_pag_alt = ciclos_pag_alt;
        this->modo = modo;
        this->implementacao = implementacao;
    }

    optional<uint16> Instrucao::buscar_endereco(Cpu* cpu)
    {
        cpu->is_pag_alterada = false;

        switch (this->modo)
        {
            case InstrucaoModo::ACM:
                return nullopt;

            case InstrucaoModo::IMPL:
                return nullopt;

            case InstrucaoModo::IMED:
                return cpu->pc + 1;

            case InstrucaoModo::P_ZERO:
                return cpu->memoria->ler(cpu->pc + 1);

            case InstrucaoModo::P_ZERO_X:
                return (cpu->memoria->ler(cpu->pc + 1) + cpu->x) & 0xFF;

            case InstrucaoModo::P_ZERO_Y:
                return (cpu->memoria->ler(cpu->pc + 1) + cpu->y) & 0xFF;

            case InstrucaoModo::ABS:
                return cpu->memoria->ler_16_bits(cpu->pc + 1);

            case InstrucaoModo::ABS_X:
            {
                uint16 endereco = cpu->memoria->ler_16_bits(cpu->pc + 1) + cpu->x;
                cpu->is_pag_alterada = !comparar_paginas(endereco - cpu->x, endereco);

                return endereco;
            }

            case InstrucaoModo::ABS_Y:
            {
                uint16 endereco =  cpu->memoria->ler_16_bits(cpu->pc + 1) + cpu->y;
                cpu->is_pag_alterada = !comparar_paginas(endereco - cpu->y, endereco);

                return endereco;
            }

            case InstrucaoModo::IND:
            {
                const uint16 valor = cpu->memoria->ler_16_bits(cpu->pc+1);
                return cpu->memoria->ler_16_bits_bug(valor);
            }

            case InstrucaoModo::IND_X:
            {
                const uint16 valor = cpu->memoria->ler(cpu->pc + 1);
                return cpu->memoria->ler_16_bits_bug((valor + cpu->x)%0x100);
            }

            case InstrucaoModo::IND_Y:
            {
                const uint16 valor = cpu->memoria->ler(cpu->pc + 1);
                uint16 endereco = cpu->memoria->ler_16_bits_bug(valor) + cpu->y;
                cpu->is_pag_alterada = !comparar_paginas(endereco - cpu->y, endereco);

                return endereco;
            }

            case InstrucaoModo::REL:
            {
                const uint16 valor = cpu->memoria->ler(cpu->pc + 1);

                if (valor < 0x80)
                    return cpu->pc + 2 + valor;
                else
                    return cpu->pc + 2 + valor - 0x100;
            }
        }

        return 0;
    }

    /*!
    Instrução ADC
    A + M + C -> A, C
    */
    static void instrucao_adc(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
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
    static void instrucao_and(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
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
    static void instrucao_asl(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
    {
        if (instrucao->modo == InstrucaoModo::ACM)
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
    static void instrucao_bcc(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
    {
        if (cpu->c == false)
        {
            cpu->branch_somar_ciclos(endereco.value());
            cpu->pc = endereco.value();
        }
    }

    //! Pula para o endereço indicado se a flag 'c' estiver ativa
    static void instrucao_bcs(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
    {
        if (cpu->c == true)
        {
            cpu->branch_somar_ciclos(endereco.value());
            cpu->pc = endereco.value();
        }
    }

    //! Pula para o endereço indicado se a flag 'z' estiver ativa
    static void instrucao_beq(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
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
    static void instrucao_bit(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
    {
        byte valor = cpu->memoria->ler(endereco.value());

        cpu->n = buscar_bit(valor, 7);
        cpu->v = buscar_bit(valor, 6);
        cpu->z = (valor & cpu->a) == 0;
    }

    //! Pula para o endereço indicado se a flag 'n' estiver ativa
    static void instrucao_bmi(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
    {
        if (cpu->n == true)
        {
            cpu->branch_somar_ciclos(endereco.value());
            cpu->pc = endereco.value();
        }
    }

    //! Pula para o endereço indicado se a flag 'z' não estiver ativa
    static void instrucao_bne(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
    {
        if (cpu->z == false)
        {
            cpu->branch_somar_ciclos(endereco.value());
            cpu->pc = endereco.value();
        }
    }

    //! Pula para o endereço indicado se a flag 'n' não estiver ativa
    static void instrucao_bpl(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
    {
        if (cpu->n == false)
        {
            cpu->branch_somar_ciclos(endereco.value());
            cpu->pc = endereco.value();
        }
    }

    //! Instrução BRK
    static void instrucao_brk(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
    {
        cpu->stack_empurrar_16_bits(cpu->pc);
        cpu->stack_empurrar(cpu->get_estado());

        cpu->b = 1;
        cpu->pc = cpu->memoria->ler_16_bits(0xFFFE);
    }

    //! Pula para o endereço indicado se a flag 'v' não estiver ativa
    static void instrucao_bvc (Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
    {
        if (cpu->v == false)
        {
            cpu->branch_somar_ciclos(endereco.value());
            cpu->pc = endereco.value();
        }
    }

    //! Pula para o endereço indicado se a flag 'v' estiver ativa
    static void instrucao_bvs(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
    {
        if (cpu->v == true)
        {
            cpu->branch_somar_ciclos(endereco.value());
            cpu->pc = endereco.value();
        }
    }

    //! Limpa a flag 'c'
    static void instrucao_clc(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
    {
        cpu->c = false;
    }

    //! Limpa a flag 'd'
    static void instrucao_cld(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
    {
        cpu->d = false;
    }

    //! Limpa a flag 'i'
    static void instrucao_cli(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
    {
        cpu->i = false;
    }

    //! Limpa a flag 'v'
    static void instrucao_clv(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
    {
        cpu->v = false;
    }

    //! Compara o acumulador com um valor
    static void instrucao_cmp(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
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
    static void instrucao_cpx(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
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
    static void instrucao_cpy(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
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
    static void instrucao_dec(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
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
    static void instrucao_dex(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
    {
        cpu->x -= 1;

        // atualizar flags
        cpu->set_n(cpu->x);
        cpu->set_z(cpu->x);
    }

    //! Diminui o valor do indice Y por 1
    static void instrucao_dey(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
    {
        cpu->y -= 1;

        // atualizar flags
        cpu->set_n(cpu->y);
        cpu->set_z(cpu->y);
    }

    //! OR exclusivo de um valor na memoria com o acumulador
    static void instrucao_eor(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
    {
        byte valor = cpu->memoria->ler(endereco.value());

        cpu->a = cpu->a ^ valor;

        //atualizar flags
        cpu->set_n(cpu->a);
        cpu->set_z(cpu->a);
    }

    //! Incrementa um valor na memoria por 1
    static void instrucao_inc(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
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
    static void instrucao_inx(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
    {
        cpu->x += 1;

        // atualizar flags
        cpu->set_n(cpu->x);
        cpu->set_z(cpu->x);
    }

    //! Incrementa o valor do indice Y por 1
    static void instrucao_iny(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
    {
        cpu->y += 1;

        // atualizar flags
        cpu->set_n(cpu->y);
        cpu->set_z(cpu->y);
    }

    //! Pula o programa para o endereço indicado
    static void instrucao_jmp(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
    {
        // muda o endereço
        cpu->pc = endereco.value();
    }

    //! Chama uma função/subrotina
    static void instrucao_jsr(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
    {
        // Salva o endereço da próxima instrução subtraído por 1 na stack.
        // O endereço guardado vai ser usado para retornar da função quando
        // o opcode 'rts' for usado
        cpu->stack_empurrar_16_bits(cpu->pc - 1);

        // muda o endereço atual do programa para o da função indicada
        cpu->pc = endereco.value();
    }

    //! Carrega um valor da memoria no acumulador
    static void instrucao_lda(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
    {
        cpu->a = cpu->memoria->ler(endereco.value());
        //std::cout << "A: " << std::bitset<8>(cpu->a) << "\n";

        // atualizar flags
        cpu->set_n(cpu->a);
        cpu->set_z(cpu->a);
    }


    //! Carrega um valor da memoria no indice X
    static void instrucao_ldx(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
    {
        cpu->x = cpu->memoria->ler(endereco.value());

        // atualizar flags
        cpu->set_n(cpu->x);
        cpu->set_z(cpu->x);
    }

    //! Carrega um valor da memoria no acumulador
    static void instrucao_ldy(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
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
    static void instrucao_lsr(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
    {
        if (instrucao->modo == InstrucaoModo::ACM)
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
    static void instrucao_nop(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
    {
    }

    //! Operanção OR entre um valor na memoria e o A
    static void instrucao_ora(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
    {
        byte valor = cpu->memoria->ler(endereco.value());

        cpu->a = cpu->a | valor;

        //atualizar flags
        cpu->set_n(cpu->a);
        cpu->set_z(cpu->a);
    }

    //! Empurra o valor do acumulador na stack
    static void instrucao_pha(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
    {
        cpu->stack_empurrar(cpu->a);
    }

    //! Empurra o valor do estado do processador na stack
    static void instrucao_php(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
    {
        const byte estado = cpu->get_estado();
        cpu->stack_empurrar(estado);
    }

    //! Puxa um valor da stack e salva esse valor no acumulador
    static void instrucao_pla(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
    {
        cpu->a = cpu->stack_puxar();

        // atualizar flags
        cpu->set_n(cpu->a);
        cpu->set_z(cpu->a);
    }

    //! Puxa um valor da stack e salva esse valor no estado do processador
    static void instrucao_plp(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
    {
        const byte estado = cpu->stack_puxar();
        cpu->set_estado(estado);
    }

    //! Gira um valor pra a esquerda
    static void instrucao_rol(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
    {
        if (instrucao->modo == InstrucaoModo::ACM)
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
    static void instrucao_ror(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
    {
        if (instrucao->modo == InstrucaoModo::ACM)
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
    static void instrucao_rti(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
    {
        const byte estado = cpu->stack_puxar();
        cpu->set_estado(estado);

        cpu->pc = cpu->stack_puxar_16_bits();
    }

    //! Retorna de uma função/sub-rotina
    static void instrucao_rts(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
    {
        cpu->pc = cpu->stack_puxar_16_bits() + 1;
    }

    //! Subtrai um valor da memoria usando o acumulador
    static void instrucao_sbc(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
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
    static void instrucao_sec(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
    {
        cpu->c = true;
    }

    //! Ativa a flag 'd'
    static void instrucao_sed(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
    {
        cpu->d = true;
    }

    //! Ativa a flag 'i'
    static void instrucao_sei(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
    {
        cpu->i = true;
    }

    //! Guarda o valor do acumulador na memoria
    static void instrucao_sta(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
    {
        cpu->memoria->escrever(endereco.value(), cpu->a);
    }

    //! Guarda o valor do registrador 'x' na memoria
    static void instrucao_stx(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
    {
        cpu->memoria->escrever(endereco.value(), cpu->x);
    }

    //! Guarda o valor do registrador 'y' na memoria
    static void instrucao_sty(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
    {
        cpu->memoria->escrever(endereco.value(), cpu->y);
    }

    //! Atribui o valor do acumulador ao registrador 'x'
    static void instrucao_tax(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
    {
        cpu->x = cpu->a;

        // atualizar flags
        cpu->set_n(cpu->x);
        cpu->set_z(cpu->x);
    }

    //! Atribui o valor do acumulador ao registrador 'y'
    static void instrucao_tay(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
    {
        cpu->y = cpu->a;

        // atualizar flags
        cpu->set_n(cpu->y);
        cpu->set_z(cpu->y);
    }

    //! Atribui o valor do ponteiro da stack ao registrador 'x'
    static void instrucao_tsx(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
    {
        cpu->x = cpu->sp;

        // atualizar flags
        cpu->set_n(cpu->x);
        cpu->set_z(cpu->x);
    }

    //! Atribui o valor do registrador 'x' ao acumulador
    static void instrucao_txa(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
    {
        cpu->a = cpu->x;

        // atualizar flags
        cpu->set_n(cpu->a);
        cpu->set_z(cpu->a);
    }

    //! Atribui o valor do registrador 'x' ao ponteiro da stack
    static void instrucao_txs(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
    {
        cpu->sp = cpu->x;
    }

    //! Atribui o valor do registrador 'y' ao acumulador
    static void instrucao_tya(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
    {
        cpu->a = cpu->y;

        // atualizar flags
        cpu->set_n(cpu->a);
        cpu->set_z(cpu->a);
    }

    //! Instrução não-oficial *DOP - nenhuma operação
    static void instrucao_dop(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
    {
    }

    //! Instrução não-oficial *TOP - nenhuma operação
    static void instrucao_top(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
    {
    }

    //! Instrução não-oficial *LAX - Transfere um valor da memória para A e X
    static void instrucao_lax(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
    {
        byte valor = cpu->memoria->ler(endereco.value());

        cpu->a = valor;
        cpu->x = valor;

        cpu->set_n(valor);
        cpu->set_z(valor);
    }

    //! Instrução não-oficial *SAX - Faz a operação AND entre o A e o X e guarda o resultado na memória
    static void instrucao_sax(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
    {
        byte valor = cpu->a & cpu->x;

        cpu->memoria->escrever(endereco.value(), valor);
    }

    //! Instrução não-oficial *DCP - Subtrai um valor da memória e compara o resultado com A
    static void instrucao_dcp(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
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
    static void instrucao_isb(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
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
    static void instrucao_slo(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
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
    static void instrucao_rla(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
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
    static void instrucao_sre(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
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
    static void instrucao_rra(Instrucao* instrucao, Cpu* cpu, optional<uint16> endereco)
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

    array< optional<Instrucao>, 256 > carregar_instrucoes()
    {
        // cria um array que será usado como uma tabela de instruções
        array< optional<Instrucao>, 256 > instrucoes;
        for (auto& instrucao : instrucoes)
        {
            instrucao = nullopt;
        }

        // modos da instrução ADC
        instrucoes.at(0x69) = Instrucao("ADC", 2, 2, 0, InstrucaoModo::IMED, instrucao_adc);
        instrucoes.at(0x65) = Instrucao("ADC", 2, 3, 0, InstrucaoModo::P_ZERO, instrucao_adc);
        instrucoes.at(0x75) = Instrucao("ADC", 2, 4, 0, InstrucaoModo::P_ZERO_X, instrucao_adc);
        instrucoes.at(0x6D) = Instrucao("ADC", 3, 4, 0, InstrucaoModo::ABS, instrucao_adc);
        instrucoes.at(0x7D) = Instrucao("ADC", 3, 4, 1, InstrucaoModo::ABS_X, instrucao_adc);
        instrucoes.at(0x79) = Instrucao("ADC", 3, 4, 1, InstrucaoModo::ABS_Y, instrucao_adc);
        instrucoes.at(0x61) = Instrucao("ADC", 2, 6, 0, InstrucaoModo::IND_X, instrucao_adc);
        instrucoes.at(0x71) = Instrucao("ADC", 2, 5, 1, InstrucaoModo::IND_Y, instrucao_adc);

        // modos da instrução AND
        instrucoes.at(0x29) = Instrucao("AND", 2, 2, 0, InstrucaoModo::IMED, instrucao_and);
        instrucoes.at(0x25) = Instrucao("AND", 2, 3, 0, InstrucaoModo::P_ZERO, instrucao_and);
        instrucoes.at(0x35) = Instrucao("AND", 2, 4, 0, InstrucaoModo::P_ZERO_X, instrucao_and);
        instrucoes.at(0x2D) = Instrucao("AND", 3, 4, 0, InstrucaoModo::ABS, instrucao_and);
        instrucoes.at(0x3D) = Instrucao("AND", 3, 4, 1, InstrucaoModo::ABS_X, instrucao_and);
        instrucoes.at(0x39) = Instrucao("AND", 3, 4, 1, InstrucaoModo::ABS_Y, instrucao_and);
        instrucoes.at(0x21) = Instrucao("AND", 2, 6, 0, InstrucaoModo::IND_X, instrucao_and);
        instrucoes.at(0x31) = Instrucao("AND", 2, 5, 1, InstrucaoModo::IND_Y, instrucao_and);

        // modos da instrução ASL
        instrucoes.at(0x0A) = Instrucao("ASL", 1, 2, 0, InstrucaoModo::ACM, instrucao_asl);
        instrucoes.at(0x06) = Instrucao("ASL", 2, 5, 0, InstrucaoModo::P_ZERO, instrucao_asl);
        instrucoes.at(0x16) = Instrucao("ASL", 2, 6, 0, InstrucaoModo::P_ZERO_X, instrucao_asl);
        instrucoes.at(0x0E) = Instrucao("ASL", 3, 6, 0, InstrucaoModo::ABS, instrucao_asl);
        instrucoes.at(0x1E) = Instrucao("ASL", 3, 7, 0, InstrucaoModo::ABS_X, instrucao_asl);

        // modos da instrução BCC
        instrucoes.at(0x90) = Instrucao("BCC", 2, 2, 0, InstrucaoModo::REL, instrucao_bcc);

        // modos da instrução BCS
        instrucoes.at(0xB0) = Instrucao("BCS", 2, 2, 0, InstrucaoModo::REL, instrucao_bcs);

        // modos da instrução BEQ
        instrucoes.at(0xF0) = Instrucao("BEQ", 2, 2, 0, InstrucaoModo::REL, instrucao_beq);

        // modos da instrução BIT
        instrucoes.at(0x24) = Instrucao("BIT", 2, 3, 0, InstrucaoModo::P_ZERO, instrucao_bit);
        instrucoes.at(0x2C) = Instrucao("BIT", 3, 4, 0, InstrucaoModo::ABS, instrucao_bit);

        // modos da instrução BMI
        instrucoes.at(0x30) = Instrucao("BMI", 2, 2, 0, InstrucaoModo::REL, instrucao_bmi);

        // modos da instrução BNE
        instrucoes.at(0xD0) = Instrucao("BNE", 2, 2, 0, InstrucaoModo::REL, instrucao_bne);

        // modos da instrução BPL
        instrucoes.at(0x10) = Instrucao("BPL", 2, 2, 0, InstrucaoModo::REL, instrucao_bpl);

        // modos da instrução BRK
        instrucoes.at(0x00) = Instrucao("BRK", 1, 7, 0, InstrucaoModo::IMPL, instrucao_brk);

        // modos da instrução BVC
        instrucoes.at(0x50) = Instrucao("BVC", 2, 2, 0, InstrucaoModo::REL, instrucao_bvc);

        // modos da instrução BVS
        instrucoes.at(0x70) = Instrucao("BVS", 2, 2, 0, InstrucaoModo::REL, instrucao_bvs);

        // modos da instrução CLC
        instrucoes.at(0x18) = Instrucao("CLC", 1, 2, 0, InstrucaoModo::IMPL, instrucao_clc);

        // modos da instrução CLD
        instrucoes.at(0xD8) = Instrucao("CLD", 1, 2, 0, InstrucaoModo::IMPL, instrucao_cld);

        // modos da instrução CLI
        instrucoes.at(0x58) = Instrucao("CLI", 1, 2, 0, InstrucaoModo::IMPL, instrucao_cli);

        // modos da instrução CLV
        instrucoes.at(0xB8) = Instrucao("CLV", 1, 2, 0, InstrucaoModo::IMPL, instrucao_clv);

        // modos da instrução CMP
        instrucoes.at(0xC9) = Instrucao("CMP", 2, 2, 0, InstrucaoModo::IMED, instrucao_cmp);
        instrucoes.at(0xC5) = Instrucao("CMP", 2, 3, 0, InstrucaoModo::P_ZERO, instrucao_cmp);
        instrucoes.at(0xD5) = Instrucao("CMP", 2, 4, 0, InstrucaoModo::P_ZERO_X, instrucao_cmp);
        instrucoes.at(0xCD) = Instrucao("CMP", 3, 4, 0, InstrucaoModo::ABS, instrucao_cmp);
        instrucoes.at(0xDD) = Instrucao("CMP", 3, 4, 1, InstrucaoModo::ABS_X, instrucao_cmp);
        instrucoes.at(0xD9) = Instrucao("CMP", 3, 4, 1, InstrucaoModo::ABS_Y, instrucao_cmp);
        instrucoes.at(0xC1) = Instrucao("CMP", 2, 6, 0, InstrucaoModo::IND_X, instrucao_cmp);
        instrucoes.at(0xD1) = Instrucao("CMP", 2, 5, 1, InstrucaoModo::IND_Y, instrucao_cmp);

        // modos da instrução CPX
        instrucoes.at(0xE0) = Instrucao("CPX", 2, 2, 0, InstrucaoModo::IMED, instrucao_cpx);
        instrucoes.at(0xE4) = Instrucao("CPX", 2, 3, 0, InstrucaoModo::P_ZERO, instrucao_cpx);
        instrucoes.at(0xEC) = Instrucao("CPX", 3, 4, 0, InstrucaoModo::ABS, instrucao_cpx);

        // modos da instrução CPY
        instrucoes.at(0xC0) = Instrucao("CPY", 2, 2, 0, InstrucaoModo::IMED, instrucao_cpy);
        instrucoes.at(0xC4) = Instrucao("CPY", 2, 3, 0, InstrucaoModo::P_ZERO, instrucao_cpy);
        instrucoes.at(0xCC) = Instrucao("CPY", 3, 4, 0, InstrucaoModo::ABS, instrucao_cpy);

        // modos da instrução DEC
        instrucoes.at(0xC6) = Instrucao("DEC", 2, 5, 0, InstrucaoModo::P_ZERO, instrucao_dec);
        instrucoes.at(0xD6) = Instrucao("DEC", 2, 6, 0, InstrucaoModo::P_ZERO_X, instrucao_dec);
        instrucoes.at(0xCE) = Instrucao("DEC", 3, 6, 0, InstrucaoModo::ABS, instrucao_dec);
        instrucoes.at(0xDE) = Instrucao("DEC", 3, 7, 0, InstrucaoModo::ABS_X, instrucao_dec);

        // modos da instrução DEX
        instrucoes.at(0xCA) = Instrucao("DEX", 1, 2, 0, InstrucaoModo::IMPL, instrucao_dex);

        // modos da instrução DEY
        instrucoes.at(0x88) = Instrucao("DEX", 1, 2, 0, InstrucaoModo::IMPL, instrucao_dey);

        // modos da instrução EOR
        instrucoes.at(0x49) = Instrucao("EOR", 2, 2, 0, InstrucaoModo::IMED, instrucao_eor);
        instrucoes.at(0x45) = Instrucao("EOR", 2, 3, 0, InstrucaoModo::P_ZERO, instrucao_eor);
        instrucoes.at(0x55) = Instrucao("EOR", 2, 4, 0, InstrucaoModo::P_ZERO_X, instrucao_eor);
        instrucoes.at(0x4D) = Instrucao("EOR", 3, 4, 0, InstrucaoModo::ABS, instrucao_eor);
        instrucoes.at(0x5D) = Instrucao("EOR", 3, 4, 1, InstrucaoModo::ABS_X, instrucao_eor);
        instrucoes.at(0x59) = Instrucao("EOR", 3, 4, 1, InstrucaoModo::ABS_Y, instrucao_eor);
        instrucoes.at(0x41) = Instrucao("EOR", 2, 6, 0, InstrucaoModo::IND_X, instrucao_eor);
        instrucoes.at(0x51) = Instrucao("EOR", 2, 5, 1, InstrucaoModo::IND_Y, instrucao_eor);

        // modos da instrução INC
        instrucoes.at(0xE6) = Instrucao("INC", 2, 5, 0, InstrucaoModo::P_ZERO, instrucao_inc);
        instrucoes.at(0xF6) = Instrucao("INC", 2, 6, 0, InstrucaoModo::P_ZERO_X, instrucao_inc);
        instrucoes.at(0xEE) = Instrucao("INC", 3, 6, 0, InstrucaoModo::ABS, instrucao_inc);
        instrucoes.at(0xFE) = Instrucao("INC", 3, 7, 0, InstrucaoModo::ABS_X, instrucao_inc);

        // modos da instrução INX
        instrucoes.at(0xE8) = Instrucao("INX", 1, 2, 0, InstrucaoModo::IMPL, instrucao_inx);

        // modos da instrução INY
        instrucoes.at(0xC8) = Instrucao("INY", 1, 2, 0, InstrucaoModo::IMPL, instrucao_iny);

        // modos da instrução JMP
        instrucoes.at(0x4C) = Instrucao("JMP", 3, 3, 0, InstrucaoModo::ABS, instrucao_jmp);
        instrucoes.at(0x6C) = Instrucao("JMP", 3, 5, 0, InstrucaoModo::IND, instrucao_jmp);

        // modos da instrução JSR
        instrucoes.at(0x20) = Instrucao("JSR", 3, 6, 0, InstrucaoModo::ABS, instrucao_jsr);

        // modos da instrução LDA
        instrucoes.at(0xA9) = Instrucao("LDA", 2, 2, 0, InstrucaoModo::IMED, instrucao_lda);
        instrucoes.at(0xA5) = Instrucao("LDA", 2, 3, 0, InstrucaoModo::P_ZERO, instrucao_lda);
        instrucoes.at(0xB5) = Instrucao("LDA", 2, 4, 0, InstrucaoModo::P_ZERO_X, instrucao_lda);
        instrucoes.at(0xAD) = Instrucao("LDA", 3, 4, 0, InstrucaoModo::ABS, instrucao_lda);
        instrucoes.at(0xBD) = Instrucao("LDA", 3, 4, 1, InstrucaoModo::ABS_X, instrucao_lda);
        instrucoes.at(0xB9) = Instrucao("LDA", 3, 4, 1, InstrucaoModo::ABS_Y, instrucao_lda);
        instrucoes.at(0xA1) = Instrucao("LDA", 2, 6, 0, InstrucaoModo::IND_X, instrucao_lda);
        instrucoes.at(0xB1) = Instrucao("LDA", 2, 5, 1, InstrucaoModo::IND_Y, instrucao_lda);

        // modos da instrução LDX
        instrucoes.at(0xA2) = Instrucao("LDX", 2, 2, 0, InstrucaoModo::IMED, instrucao_ldx);
        instrucoes.at(0xA6) = Instrucao("LDX", 2, 3, 0, InstrucaoModo::P_ZERO, instrucao_ldx);
        instrucoes.at(0xB6) = Instrucao("LDX", 2, 4, 0, InstrucaoModo::P_ZERO_Y, instrucao_ldx);
        instrucoes.at(0xAE) = Instrucao("LDX", 3, 4, 0, InstrucaoModo::ABS, instrucao_ldx);
        instrucoes.at(0xBE) = Instrucao("LDX", 3, 4, 1, InstrucaoModo::ABS_Y, instrucao_ldx);

        // modos da instrução LDY
        instrucoes.at(0xA0) = Instrucao("LDY", 2, 2, 0, InstrucaoModo::IMED, instrucao_ldy);
        instrucoes.at(0xA4) = Instrucao("LDY", 2, 3, 0, InstrucaoModo::P_ZERO, instrucao_ldy);
        instrucoes.at(0xB4) = Instrucao("LDY", 2, 4, 0, InstrucaoModo::P_ZERO_X, instrucao_ldy);
        instrucoes.at(0xAC) = Instrucao("LDY", 3, 4, 0, InstrucaoModo::ABS, instrucao_ldy);
        instrucoes.at(0xBC) = Instrucao("LDY", 3, 4, 1, InstrucaoModo::ABS_X, instrucao_ldy);

        // modos da instrução LSR
        instrucoes.at(0x4A) = Instrucao("LSR", 1, 2, 0, InstrucaoModo::ACM, instrucao_lsr);
        instrucoes.at(0x46) = Instrucao("LSR", 2, 5, 0, InstrucaoModo::P_ZERO, instrucao_lsr);
        instrucoes.at(0x56) = Instrucao("LSR", 2, 6, 0, InstrucaoModo::P_ZERO_X, instrucao_lsr);
        instrucoes.at(0x4E) = Instrucao("LSR", 3, 6, 0, InstrucaoModo::ABS, instrucao_lsr);
        instrucoes.at(0x5E) = Instrucao("LSR", 3, 7, 0, InstrucaoModo::ABS_X, instrucao_lsr);

        // modos da instrução NOP
        instrucoes.at(0xEA) = Instrucao("NOP", 1, 2, 0, InstrucaoModo::IMPL, instrucao_nop);
        // opcodes não-oficiais da instrução NOP
        instrucoes.at(0x1A) = Instrucao("*NOP", 1, 2, 0, InstrucaoModo::IMPL, instrucao_nop);
        instrucoes.at(0x3A) = Instrucao("*NOP", 1, 2, 0, InstrucaoModo::IMPL, instrucao_nop);
        instrucoes.at(0x5A) = Instrucao("*NOP", 1, 2, 0, InstrucaoModo::IMPL, instrucao_nop);
        instrucoes.at(0x7A) = Instrucao("*NOP", 1, 2, 0, InstrucaoModo::IMPL, instrucao_nop);
        instrucoes.at(0xDA) = Instrucao("*NOP", 1, 2, 0, InstrucaoModo::IMPL, instrucao_nop);
        instrucoes.at(0xFA) = Instrucao("*NOP", 1, 2, 0, InstrucaoModo::IMPL, instrucao_nop);

        // modos da instrução ORA
        instrucoes.at(0x09) = Instrucao("ORA", 2, 2, 0, InstrucaoModo::IMED, instrucao_ora);
        instrucoes.at(0x05) = Instrucao("ORA", 2, 3, 0, InstrucaoModo::P_ZERO, instrucao_ora);
        instrucoes.at(0x15) = Instrucao("ORA", 2, 4, 0, InstrucaoModo::P_ZERO_X, instrucao_ora);
        instrucoes.at(0x0D) = Instrucao("ORA", 3, 4, 0, InstrucaoModo::ABS, instrucao_ora);
        instrucoes.at(0x1D) = Instrucao("ORA", 3, 4, 1, InstrucaoModo::ABS_X, instrucao_ora);
        instrucoes.at(0x19) = Instrucao("ORA", 3, 4, 1, InstrucaoModo::ABS_Y, instrucao_ora);
        instrucoes.at(0x01) = Instrucao("ORA", 2, 6, 0, InstrucaoModo::IND_X, instrucao_ora);
        instrucoes.at(0x11) = Instrucao("ORA", 2, 5, 1, InstrucaoModo::IND_Y, instrucao_ora);

        // modos da instrução PHA
        instrucoes.at(0x48) = Instrucao("PHA", 1, 3, 0, InstrucaoModo::IMPL, instrucao_pha);

        // modos da instrução PHP
        instrucoes.at(0x08) = Instrucao("PHP", 1, 3, 0, InstrucaoModo::IMPL, instrucao_php);

        // modos da instrução PLA
        instrucoes.at(0x68) = Instrucao("PLA", 1, 4, 0, InstrucaoModo::IMPL, instrucao_pla);

        // modos da instrução PLP
        instrucoes.at(0x28) = Instrucao("PLP", 1, 4, 0, InstrucaoModo::IMPL, instrucao_plp);

        // modos da instrução ROL
        instrucoes.at(0x2A) = Instrucao("ROL", 1, 2, 0, InstrucaoModo::ACM, instrucao_rol);
        instrucoes.at(0x26) = Instrucao("ROL", 2, 5, 0, InstrucaoModo::P_ZERO, instrucao_rol);
        instrucoes.at(0x36) = Instrucao("ROL", 2, 6, 0, InstrucaoModo::P_ZERO_X, instrucao_rol);
        instrucoes.at(0x2E) = Instrucao("ROL", 3, 6, 0, InstrucaoModo::ABS, instrucao_rol);
        instrucoes.at(0x3E) = Instrucao("ROL", 3, 7, 0, InstrucaoModo::ABS_X, instrucao_rol);

        // modos da instrução ROR
        instrucoes.at(0x6A) = Instrucao("ROR", 1, 2, 0, InstrucaoModo::ACM, instrucao_ror);
        instrucoes.at(0x66) = Instrucao("ROR", 2, 5, 0, InstrucaoModo::P_ZERO, instrucao_ror);
        instrucoes.at(0x76) = Instrucao("ROR", 2, 6, 0, InstrucaoModo::P_ZERO_X, instrucao_ror);
        instrucoes.at(0x6E) = Instrucao("ROR", 3, 6, 0, InstrucaoModo::ABS, instrucao_ror);
        instrucoes.at(0x7E) = Instrucao("ROR", 3, 7, 0, InstrucaoModo::ABS_X, instrucao_ror);

        // modos da instrução RTI
        instrucoes.at(0x40) = Instrucao("RTI", 1, 6, 0, InstrucaoModo::IMPL, instrucao_rti);

        // modos da instrução RTS
        instrucoes.at(0x60) = Instrucao("RTS", 1, 6, 0, InstrucaoModo::IMPL, instrucao_rts);

        // modos da instrução SBC
        instrucoes.at(0xE9) = Instrucao("SBC", 2, 2, 0, InstrucaoModo::IMED, instrucao_sbc);
        instrucoes.at(0xE5) = Instrucao("SBC", 2, 3, 0, InstrucaoModo::P_ZERO, instrucao_sbc);
        instrucoes.at(0xF5) = Instrucao("SBC", 2, 4, 0, InstrucaoModo::P_ZERO_X, instrucao_sbc);
        instrucoes.at(0xED) = Instrucao("SBC", 3, 4, 0, InstrucaoModo::ABS, instrucao_sbc);
        instrucoes.at(0xFD) = Instrucao("SBC", 3, 4, 1, InstrucaoModo::ABS_X, instrucao_sbc);
        instrucoes.at(0xF9) = Instrucao("SBC", 3, 4, 1, InstrucaoModo::ABS_Y, instrucao_sbc);
        instrucoes.at(0xE1) = Instrucao("SBC", 2, 6, 0, InstrucaoModo::IND_X, instrucao_sbc);
        instrucoes.at(0xF1) = Instrucao("SBC", 2, 5, 1, InstrucaoModo::IND_Y, instrucao_sbc);
        // opcode não-oficial da instrução SBC
        instrucoes.at(0xEB) = Instrucao("*SBC", 2, 2, 0, InstrucaoModo::IMED, instrucao_sbc);

        // modos da instrução SEC
        instrucoes.at(0x38) = Instrucao("SEC", 1, 2, 0, InstrucaoModo::IMPL, instrucao_sec);

        // modos da instrução SED
        instrucoes.at(0xF8) = Instrucao("SED", 1, 2, 0, InstrucaoModo::IMPL, instrucao_sed);

        // modos da instrução SEI
        instrucoes.at(0x78) = Instrucao("SEI", 1, 2, 0, InstrucaoModo::IMPL, instrucao_sei);

        // modos da instrução STA
        instrucoes.at(0x85) = Instrucao("STA", 2, 3, 0, InstrucaoModo::P_ZERO, instrucao_sta);
        instrucoes.at(0x95) = Instrucao("STA", 2, 4, 0, InstrucaoModo::P_ZERO_X, instrucao_sta);
        instrucoes.at(0x8D) = Instrucao("STA", 3, 4, 0, InstrucaoModo::ABS, instrucao_sta);
        instrucoes.at(0x9D) = Instrucao("STA", 3, 5, 0, InstrucaoModo::ABS_X, instrucao_sta);
        instrucoes.at(0x99) = Instrucao("STA", 3, 5, 0, InstrucaoModo::ABS_Y, instrucao_sta);
        instrucoes.at(0x81) = Instrucao("STA", 2, 6, 0, InstrucaoModo::IND_X, instrucao_sta);
        instrucoes.at(0x91) = Instrucao("STA", 2, 6, 0, InstrucaoModo::IND_Y, instrucao_sta);

        // modos da instrução STX
        instrucoes.at(0x86) = Instrucao("STX", 2, 3, 0, InstrucaoModo::P_ZERO, instrucao_stx);
        instrucoes.at(0x96) = Instrucao("STX", 2, 4, 0, InstrucaoModo::P_ZERO_Y, instrucao_stx);
        instrucoes.at(0x8E) = Instrucao("STX", 3, 4, 0, InstrucaoModo::ABS, instrucao_stx);

        // modos da instrução STY
        instrucoes.at(0x84) = Instrucao("STY", 2, 3, 0, InstrucaoModo::P_ZERO, instrucao_sty);
        instrucoes.at(0x94) = Instrucao("STY", 2, 4, 0, InstrucaoModo::P_ZERO_X, instrucao_sty);
        instrucoes.at(0x8C) = Instrucao("STY", 3, 4, 0, InstrucaoModo::ABS, instrucao_sty);

        // modos da instrução TAX
        instrucoes.at(0xAA) = Instrucao("TAX", 1, 2, 0, InstrucaoModo::IMPL, instrucao_tax);

        // modos da instrução TAY
        instrucoes.at(0xA8) = Instrucao("TAY", 1, 2, 0, InstrucaoModo::IMPL, instrucao_tay);

        // modos da instrução TSX
        instrucoes.at(0xBA) = Instrucao("TSX", 1, 2, 0, InstrucaoModo::IMPL, instrucao_tsx);

        // modos da instrução TXA
        instrucoes.at(0x8A) = Instrucao("TXA", 1, 2, 0, InstrucaoModo::IMPL, instrucao_txa);

        // modos da instrução TXS
        instrucoes.at(0x9A) = Instrucao("TXS", 1, 2, 0, InstrucaoModo::IMPL, instrucao_txs);

        // modos da instrução TYA
        instrucoes.at(0x98) = Instrucao("TYA", 1, 2, 0, InstrucaoModo::IMPL, instrucao_tya);

        // modos da instrução não-oficial *DOP
        instrucoes.at(0x04) = Instrucao("*DOP", 2, 3, 0, InstrucaoModo::P_ZERO, instrucao_dop);
        instrucoes.at(0x14) = Instrucao("*DOP", 2, 4, 0, InstrucaoModo::P_ZERO_X, instrucao_dop);
        instrucoes.at(0x34) = Instrucao("*DOP", 2, 4, 0, InstrucaoModo::P_ZERO_X, instrucao_dop);
        instrucoes.at(0x44) = Instrucao("*DOP", 2, 3, 0, InstrucaoModo::P_ZERO, instrucao_dop);
        instrucoes.at(0x54) = Instrucao("*DOP", 2, 4, 0, InstrucaoModo::P_ZERO_X, instrucao_dop);
        instrucoes.at(0x64) = Instrucao("*DOP", 2, 3, 0, InstrucaoModo::P_ZERO, instrucao_dop);
        instrucoes.at(0x74) = Instrucao("*DOP", 2, 4, 0, InstrucaoModo::P_ZERO_X, instrucao_dop);
        instrucoes.at(0x80) = Instrucao("*DOP", 2, 2, 0, InstrucaoModo::IMED, instrucao_dop);
        instrucoes.at(0x82) = Instrucao("*DOP", 2, 2, 0, InstrucaoModo::IMED, instrucao_dop);
        instrucoes.at(0x89) = Instrucao("*DOP", 2, 2, 0, InstrucaoModo::IMED, instrucao_dop);
        instrucoes.at(0xC2) = Instrucao("*DOP", 2, 2, 0, InstrucaoModo::IMED, instrucao_dop);
        instrucoes.at(0xD4) = Instrucao("*DOP", 2, 4, 0, InstrucaoModo::P_ZERO_X, instrucao_dop);
        instrucoes.at(0xE2) = Instrucao("*DOP", 2, 2, 0, InstrucaoModo::IMED, instrucao_dop);
        instrucoes.at(0xF4) = Instrucao("*DOP", 2, 4, 0, InstrucaoModo::P_ZERO_X, instrucao_dop);

        // modos da instrução não-oficial *TOP
        instrucoes.at(0x0C) = Instrucao("*TOP", 3, 4, 0, InstrucaoModo::ABS, instrucao_top);
        instrucoes.at(0x1C) = Instrucao("*TOP", 3, 4, 1, InstrucaoModo::ABS_X, instrucao_top);
        instrucoes.at(0x3C) = Instrucao("*TOP", 3, 4, 1, InstrucaoModo::ABS_X, instrucao_top);
        instrucoes.at(0x5C) = Instrucao("*TOP", 3, 4, 1, InstrucaoModo::ABS_X, instrucao_top);
        instrucoes.at(0x7C) = Instrucao("*TOP", 3, 4, 1, InstrucaoModo::ABS_X, instrucao_top);
        instrucoes.at(0xDC) = Instrucao("*TOP", 3, 4, 1, InstrucaoModo::ABS_X, instrucao_top);
        instrucoes.at(0xFC) = Instrucao("*TOP", 3, 4, 1, InstrucaoModo::ABS_X, instrucao_top);

        // modos da instrução não-oficial *LAX
        instrucoes.at(0xA7) = Instrucao("*LAX", 2, 3, 0, InstrucaoModo::P_ZERO, instrucao_lax);
        instrucoes.at(0xB7) = Instrucao("*LAX", 2, 4, 0, InstrucaoModo::P_ZERO_Y, instrucao_lax);
        instrucoes.at(0xAF) = Instrucao("*LAX", 3, 4, 0, InstrucaoModo::ABS, instrucao_lax);
        instrucoes.at(0xBF) = Instrucao("*LAX", 3, 4, 1, InstrucaoModo::ABS_Y, instrucao_lax);
        instrucoes.at(0xA3) = Instrucao("*LAX", 2, 6, 0, InstrucaoModo::IND_X, instrucao_lax);
        instrucoes.at(0xB3) = Instrucao("*LAX", 2, 5, 1, InstrucaoModo::IND_Y, instrucao_lax);

        // modos da instrução não-oficial *SAX
        instrucoes.at(0x87) = Instrucao("*SAX", 2, 3, 0, InstrucaoModo::P_ZERO, instrucao_sax);
        instrucoes.at(0x97) = Instrucao("*SAX", 2, 4, 0, InstrucaoModo::P_ZERO_Y, instrucao_sax);
        instrucoes.at(0x83) = Instrucao("*SAX", 2, 6, 0, InstrucaoModo::IND_X, instrucao_sax);
        instrucoes.at(0x8F) = Instrucao("*SAX", 3, 4, 0, InstrucaoModo::ABS, instrucao_sax);

        // modos da instrução não-oficial *DCP
        instrucoes.at(0xC7) = Instrucao("*DCP", 2, 5, 0, InstrucaoModo::P_ZERO, instrucao_dcp);
        instrucoes.at(0xD7) = Instrucao("*DCP", 2, 6, 0, InstrucaoModo::P_ZERO_X, instrucao_dcp);
        instrucoes.at(0xCF) = Instrucao("*DCP", 3, 6, 0, InstrucaoModo::ABS, instrucao_dcp);
        instrucoes.at(0xDF) = Instrucao("*DCP", 3, 7, 0, InstrucaoModo::ABS_X, instrucao_dcp);
        instrucoes.at(0xDB) = Instrucao("*DCP", 3, 7, 0, InstrucaoModo::ABS_Y, instrucao_dcp);
        instrucoes.at(0xC3) = Instrucao("*DCP", 2, 8, 0, InstrucaoModo::IND_X, instrucao_dcp);
        instrucoes.at(0xD3) = Instrucao("*DCP", 2, 8, 0, InstrucaoModo::IND_Y, instrucao_dcp);

        // modos da instrução não-oficial *ISB
        instrucoes.at(0xE7) = Instrucao("*ISB", 2, 5, 0, InstrucaoModo::P_ZERO, instrucao_isb);
        instrucoes.at(0xF7) = Instrucao("*ISB", 2, 6, 0, InstrucaoModo::P_ZERO_X, instrucao_isb);
        instrucoes.at(0xEF) = Instrucao("*ISB", 3, 6, 0, InstrucaoModo::ABS, instrucao_isb);
        instrucoes.at(0xFF) = Instrucao("*ISB", 3, 7, 0, InstrucaoModo::ABS_X, instrucao_isb);
        instrucoes.at(0xFB) = Instrucao("*ISB", 3, 7, 0, InstrucaoModo::ABS_Y, instrucao_isb);
        instrucoes.at(0xE3) = Instrucao("*ISB", 2, 8, 0, InstrucaoModo::IND_X, instrucao_isb);
        instrucoes.at(0xF3) = Instrucao("*ISB", 2, 8, 0, InstrucaoModo::IND_Y, instrucao_isb);

        // modos da instrução não-oficial *SLO
        instrucoes.at(0x07) = Instrucao("*SLO", 2, 5, 0, InstrucaoModo::P_ZERO, instrucao_slo);
        instrucoes.at(0x17) = Instrucao("*SLO", 2, 6, 0, InstrucaoModo::P_ZERO_X, instrucao_slo);
        instrucoes.at(0x0F) = Instrucao("*SLO", 3, 6, 0, InstrucaoModo::ABS, instrucao_slo);
        instrucoes.at(0x1F) = Instrucao("*SLO", 3, 7, 0, InstrucaoModo::ABS_X, instrucao_slo);
        instrucoes.at(0x1B) = Instrucao("*SLO", 3, 7, 0, InstrucaoModo::ABS_Y, instrucao_slo);
        instrucoes.at(0x03) = Instrucao("*SLO", 2, 8, 0, InstrucaoModo::IND_X, instrucao_slo);
        instrucoes.at(0x13) = Instrucao("*SLO", 2, 8, 0, InstrucaoModo::IND_Y, instrucao_slo);

        // modos da instrução não-oficial *RLA
        instrucoes.at(0x27) = Instrucao("*RLA", 2, 5, 0, InstrucaoModo::P_ZERO, instrucao_rla);
        instrucoes.at(0x37) = Instrucao("*RLA", 2, 6, 0, InstrucaoModo::P_ZERO_X, instrucao_rla);
        instrucoes.at(0x2F) = Instrucao("*RLA", 3, 6, 0, InstrucaoModo::ABS, instrucao_rla);
        instrucoes.at(0x3F) = Instrucao("*RLA", 3, 7, 0, InstrucaoModo::ABS_X, instrucao_rla);
        instrucoes.at(0x3B) = Instrucao("*RLA", 3, 7, 0, InstrucaoModo::ABS_Y, instrucao_rla);
        instrucoes.at(0x23) = Instrucao("*RLA", 2, 8, 0, InstrucaoModo::IND_X, instrucao_rla);
        instrucoes.at(0x33) = Instrucao("*RLA", 2, 8, 0, InstrucaoModo::IND_Y, instrucao_rla);

        // modos da instrução não-oficial *SRE
        instrucoes.at(0x47) = Instrucao("*SRE", 2, 5, 0, InstrucaoModo::P_ZERO, instrucao_sre);
        instrucoes.at(0x57) = Instrucao("*SRE", 2, 6, 0, InstrucaoModo::P_ZERO_X, instrucao_sre);
        instrucoes.at(0x4F) = Instrucao("*SRE", 3, 6, 0, InstrucaoModo::ABS, instrucao_sre);
        instrucoes.at(0x5F) = Instrucao("*SRE", 3, 7, 0, InstrucaoModo::ABS_X, instrucao_sre);
        instrucoes.at(0x5B) = Instrucao("*SRE", 3, 7, 0, InstrucaoModo::ABS_Y, instrucao_sre);
        instrucoes.at(0x43) = Instrucao("*SRE", 2, 8, 0, InstrucaoModo::IND_X, instrucao_sre);
        instrucoes.at(0x53) = Instrucao("*SRE", 2, 8, 0, InstrucaoModo::IND_Y, instrucao_sre);

        // modos da instrução não-oficial *RRA
        instrucoes.at(0x67) = Instrucao("*RRA", 2, 5, 0, InstrucaoModo::P_ZERO, instrucao_rra);
        instrucoes.at(0x77) = Instrucao("*RRA", 2, 6, 0, InstrucaoModo::P_ZERO_X, instrucao_rra);
        instrucoes.at(0x6F) = Instrucao("*RRA", 3, 6, 0, InstrucaoModo::ABS, instrucao_rra);
        instrucoes.at(0x7F) = Instrucao("*RRA", 3, 7, 0, InstrucaoModo::ABS_X, instrucao_rra);
        instrucoes.at(0x7B) = Instrucao("*RRA", 3, 7, 0, InstrucaoModo::ABS_Y, instrucao_rra);
        instrucoes.at(0x63) = Instrucao("*RRA", 2, 8, 0, InstrucaoModo::IND_X, instrucao_rra);
        instrucoes.at(0x73) = Instrucao("*RRA", 2, 8, 0, InstrucaoModo::IND_Y, instrucao_rra);

        return instrucoes;
    }
}