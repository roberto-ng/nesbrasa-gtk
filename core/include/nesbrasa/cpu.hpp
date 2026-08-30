#pragma once
#include <array>
#include <optional>
#include <string>
#include <sstream>
#include <nesbrasa/tipos_numeros.hpp>
#include <nesbrasa/ports.hpp>
#include <nesbrasa/instrucao.hpp>
#include <nesbrasa/util.hpp>

namespace nesbrasa::nucleo
{
    std::array<std::optional<Instrucao>, 256> carregar_instrucoes();
    class Cpu
    {
        tipos::uint16 esperar;
        tipos::uint32 ciclos;
        std::array<std::optional<Instrucao>, 256> instrucoes;

        void executar(Instrucao*);
        void executar_operacao(InstrucaoOperacao, InstrucaoModo, std::optional<tipos::uint16>);
        std::optional<tipos::uint16> buscar_endereco(InstrucaoModo);

    public:
        CpuBus* memoria;
        Interrupcao interrupcao;
        tipos::uint16 pc;
        tipos::byte sp, a, x, y;
        bool c, z, i, d, b, v, n, is_pag_alterada;

        explicit Cpu(CpuBus*);
        tipos::uint avancar();
        void resetar();
        void branch_somar_ciclos(tipos::uint16);
        tipos::byte get_estado();
        void set_estado(tipos::byte);
        void stack_empurrar(tipos::byte);
        void stack_empurrar_16_bits(tipos::uint16);
        tipos::byte stack_puxar();
        tipos::uint16 stack_puxar_16_bits();
        void esperar_adicionar(tipos::uint16);
        void set_z(tipos::byte);
        void set_n(tipos::byte);
        tipos::uint32 get_ciclos();
        tipos::uint16 get_esperar();
        std::optional<Instrucao> get_instrucao(tipos::byte);
        std::string instrucao_para_asm(tipos::byte);
    };
}
