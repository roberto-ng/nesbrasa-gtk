/* cpu.hpp
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

#pragma once

#include <cstdint>
#include <array>
#include <memory>
#include <optional>

#include "instrucao.hpp"
#include "memoria.hpp"

// referencias utilizadas:
// http://www.obelisk.me.uk/6502/registers.html

namespace nesbrasa::nucleo
{
    using std::array;
    using std::shared_ptr;
    using std::optional;

    enum class Interrupcao
    {
        NMI,
        IRQ,
        NENHUMA,
    };

    class Cpu
    {
    private:
        uint16 esperar;
        uint32 ciclos;

        // tabela de instruções
        array< optional<Instrucao>, 256 > instrucoes;
    
    public:

        Memoria* memoria;

        Interrupcao interrupcao;

        uint16 pc; // contador de programa
        byte   sp; // ponteiro da stack

        byte a; // registrador acumulador
        byte x; // registrador de indice x
        byte y; // registrador de indice y

        bool c; // flag de carregamento (carry flag)
        bool z; // flag zero
        bool i; // flag de desabilitar interrupções
        bool d; // flag decimal
        bool b; // flag da instrução break (break command flag)
        bool v; // flag de transbordamento (overflow flag)
        bool n; // flag de negativo

        bool is_pag_alterada;

        Cpu(Memoria* memoria);

        /*! Executa a próxima instrução
            \return Quantidade de ciclos que foram executados
         */  
        uint avancar();

        void resetar();

        /*! Calcula a quantidade de ciclos em um branch e a soma em 'cpu->ciclos'.
            \param endereco O endereço em que o branch sera realizado
        */
        void branch_somar_ciclos(uint16 endereco);

        byte get_estado();

        void set_estado(byte valor);

        //! Empurra um valor na stack
        void stack_empurrar(byte valor);

        //! Empurra um valor na stack
        void stack_empurrar_16_bits(uint16 valor);

        //! Puxa um valor da stack
        byte stack_puxar();

        //! Puxa um valor da stack
        uint16 stack_puxar_16_bits();

        void esperar_adicionar(uint16 esperar);

        string instrucao_para_asm(byte opcode);

        //! Ativa a flag de zero caso seja necessario
        void set_z(byte valor);

        //! Ativa a flag de valor negativo caso seja necessario
        void set_n(byte valor);
        
        uint32 get_ciclos();

        uint16 get_esperar();

        optional<Instrucao> get_instrucao(byte opcode);

    private:
        void executar(Instrucao* instrucao);
    };
}