module;
#include <array>
export module nesbrasa.controller;
import nesbrasa.types;

export namespace nesbrasa::nucleo
{
    enum class Botao : tipos::uint { A, B, SELECT, START, CIMA, BAIXO, ESQUERDA, DIREITA };

    class Controle
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
