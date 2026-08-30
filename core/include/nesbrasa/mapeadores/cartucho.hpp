#pragma once
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <nesbrasa/tipos_numeros.hpp>
#include <nesbrasa/ports.hpp>

namespace nesbrasa::nucleo::mapeadores
{
    enum class CartuchoTipo { NROM = 0, MMC1 = 1, DESCONHECIDO };
    enum class ArquivoFormato { DESCONHECIDO, INES, NES_2_0 };
    class Cartucho;
    using CartuchoCriador = std::unique_ptr<Cartucho>(*)(CartuchoTipo, int, int, std::vector<tipos::byte>&, ArquivoFormato, tipos::byte);
    extern CartuchoCriador criador_nrom;

    class Cartucho : public CartridgePort
    {
    protected:
        int prg_bancos_quantidade, chr_bancos_quantidade;
        bool possui_prg_ram, possui_chr_ram;
        std::vector<tipos::byte> rom_prg, rom_chr, ram_prg, ram_chr;
    public:
        ArquivoFormato arquivo_formato;
        tipos::byte espelhamento;
        static constexpr int PRG_BANCOS_TAMANHO = 0x4000;
        static constexpr int CHR_BANCOS_TAMANHO = 0x2000;
        static std::unique_ptr<Cartucho> criar(CartuchoTipo, int, int, std::vector<tipos::byte>&, ArquivoFormato, tipos::byte);
        Cartucho(int, int, std::vector<tipos::byte>&, ArquivoFormato, tipos::byte);
        virtual ~Cartucho() = default;
        tipos::byte ler(tipos::uint16) override = 0;
        void escrever(tipos::uint16, tipos::byte) override = 0;
        virtual std::string get_nome() = 0;
        int get_prg_bancos_quantidade();
        int get_chr_bancos_quantidade();
        tipos::byte get_espelhamento() const override;
    };
    void registrar_criador_nrom(CartuchoCriador);
}

