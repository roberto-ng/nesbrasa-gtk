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
        Controle()
        {
            this->buffer_botoes.fill(false);
            this->indice = 0;
            this->sinal = false;
        }

        tipos::byte ler()
        {
            tipos::byte valor = 0;
            if (this->indice < 8 && this->buffer_botoes.at(this->indice) == true)
            {
                valor = 1;
            }

            if ((this->sinal & 1) != 0)
            {
                this->indice = 0;
            }
            else
            {
                this->indice += 1;
            }

            return valor;
        }

        void escrever(tipos::byte valor)
        {
            this->sinal = valor;

            if ((this->sinal & 1) != 0)
            {
                this->indice = 0;
            }
        }

        void set_valor(Botao botao, bool valor)
        {
            tipos::uint botao_indice = static_cast<tipos::uint>(botao);
            this->buffer_botoes.at(botao_indice) = valor;
        }
    };
}

