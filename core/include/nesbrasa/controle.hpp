#pragma once
#include <array>
#include <nesbrasa/tipos_numeros.hpp>
#include <nesbrasa/ports.hpp>

namespace nesbrasa::nucleo
{
    enum class Botao : tipos::uint { A, B, SELECT, START, CIMA, BAIXO, ESQUERDA, DIREITA };

    class Controle : public ControllerPort
    {
        std::array<bool, 8> buffer_botoes;
        tipos::byte indice, sinal;

    public:
        Controle();
        tipos::byte ler() override;
        void escrever(tipos::byte valor) override;
        void set_valor(Botao botao, bool valor);
    };
}

