#include <nesbrasa/controle.hpp>

namespace nesbrasa::nucleo
{
Controle::Controle() {

            this->buffer_botoes.fill(false);
            this->indice = 0;
            this->sinal = false;
        
}

tipos::byte Controle::ler() {

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

void Controle::escrever(tipos::byte valor) {

            this->sinal = valor;

            if ((this->sinal & 1) != 0)
            {
                this->indice = 0;
            }
        
}

void Controle::set_valor(Botao botao, bool valor) {

            tipos::uint botao_indice = static_cast<tipos::uint>(botao);
            this->buffer_botoes.at(botao_indice) = valor;
        
}
}

