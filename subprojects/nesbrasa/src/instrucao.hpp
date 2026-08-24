/* instrucao.hpp
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

#include <functional>
#include <string>
#include <array>
#include <optional>

#include "cpu.hpp"
#include "tipos_numeros.hpp"

// referencias utilizadas:
// https://www.masswerk.at/6502/6502_instruction_set.html
// http://nesdev.com/undocumented_opcodes.txt

namespace nesbrasa::nucleo
{
    using std::function;
    using std::string;
    using std::array;
    using std::shared_ptr;
    using std::optional;
    using std::nullopt;
    using namespace nesbrasa::tipos;

    class Cpu;
    class Instrucao;

    // Tipo usado para referenciar funções de alto nível 
    // que reimplementam instruções da arquitetura 6502
    using InstrucaoImplementacao = std::function< void(Instrucao*, Cpu*, optional<uint16>) >;

    //! Modos de endereçamento das instruções
    enum class InstrucaoModo
    {
        ACM,       // acumulador
        ABS,       // absoluto
        ABS_X,     // absoluto, indexado pelo registrador x
        ABS_Y,     // absoluto, indexado pelo registrador y
        IMED,      // imediato
        IMPL,      // implícito
        IND,       // indireto
        IND_X,     // indireto indexado por x
        IND_Y,     // indireto indexado por y
        REL,       // relativo
        P_ZERO,    // página 0
        P_ZERO_X,  // página 0, indexado pelo registrador x
        P_ZERO_Y,  // página 0, indexado pelo registrador y
    };

    class Instrucao
    {
    public:
        string nome;
        byte   bytes;
        int32  ciclos;

        // quantidade de ciclos adicionais que devem ocorrer quando a
        // página da memoria for alterada durante a leitura do endereço
        int32 ciclos_pag_alt;

        InstrucaoModo modo;

        /*! Uma fução de alto nivel que sera usada para reimplementar
            uma instrução da arquitetura 6502 */
        InstrucaoImplementacao implementacao;
      
        Instrucao(
            string nome,
            byte bytes,
            int32 ciclos,
            int32 ciclos_pag_alt,
            InstrucaoModo modo,
            InstrucaoImplementacao implementacao
        );

        /*!
        Busca o endereço que vai ser usado por uma instrução de
        acordo com o modo de endereçamento da CPU
        */
        optional<uint16> buscar_endereco(Cpu* cpu);
    };

    array< optional<Instrucao>, 256 > carregar_instrucoes();
}