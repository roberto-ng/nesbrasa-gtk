#pragma once
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <nesbrasa/mapeadores/cartucho.hpp>

namespace nesbrasa::nucleo::mapeadores
{
    class NRom : public Cartucho
    {
    public:
        NRom(int, int, std::vector<tipos::byte>&, ArquivoFormato, tipos::byte);
        tipos::byte ler(tipos::uint16) override;
        void escrever(tipos::uint16, tipos::byte) override;
        std::string get_nome() override;
    };
    void registrar_nrom();
}

