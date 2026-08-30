#include <nesbrasa/instrucao.hpp>
#include <utility>

namespace nesbrasa::nucleo
{
    Instrucao::Instrucao(std::string nome, tipos::byte bytes, tipos::int32 ciclos,
                         tipos::int32 ciclos_pag_alt, InstrucaoModo modo,
                         InstrucaoOperacao operacao):
        nome(std::move(nome)), bytes(bytes), ciclos(ciclos), ciclos_pag_alt(ciclos_pag_alt),
        modo(modo), operacao(operacao) {}
}
