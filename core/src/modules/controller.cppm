module;
#include <array>
export module nesbrasa.controller;
import nesbrasa.types;
import nesbrasa.ports;

export namespace nesbrasa::nucleo
{
    enum class Botao : tipos::uint { A, B, SELECT, START, CIMA, BAIXO, ESQUERDA, DIREITA };

    class Controle : public ControllerPort
    {
        std::array<bool, 8> buffer_botoes;
        tipos::byte indice, sinal;
    public:
        Controle();
        tipos::byte ler();
        void escrever(tipos::byte valor);
        void set_valor(Botao botao, bool valor);
    };
}


using namespace nesbrasa::tipos;

namespace nesbrasa::nucleo
{
    Controle::Controle()
    {
        this->buffer_botoes.fill(false);
        this->indice = 0;
        this->sinal = false;
    }

    byte Controle::ler()
    {
        byte valor = 0;
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

    void Controle::escrever(byte valor)
    {
        this->sinal = valor;

        if ((this->sinal & 1) != 0)
        {
            this->indice = 0;
        }
    }

    void Controle::set_valor(Botao botao, bool valor)
    {
        uint botao_indice = static_cast<uint>(botao);
        this->buffer_botoes.at(botao_indice) = valor;
    }
}
