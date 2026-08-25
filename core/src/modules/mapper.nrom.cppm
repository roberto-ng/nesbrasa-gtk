module;
#include <stdexcept>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
export module nesbrasa.mapper.nrom;

import nesbrasa.cartridge;
import nesbrasa.types;

export namespace nesbrasa::nucleo::mapeadores
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


using namespace nesbrasa::tipos;
using std::vector;
using std::string;

namespace nesbrasa::nucleo::mapeadores
{
    using std::runtime_error;
    using std::make_unique;
    using namespace std::string_literals;
    using namespace nucleo;

    std::unique_ptr<Cartucho> criar_nrom(CartuchoTipo, int prg_qtd, int chr_qtd,
                                         vector<byte>& arquivo, ArquivoFormato formato,
                                         byte espelhamento)
    {
        return make_unique<NRom>(prg_qtd, chr_qtd, arquivo, formato, espelhamento);
    }

    void registrar_nrom()
    {
        registrar_criador_nrom(&criar_nrom);
    }

    NRom::NRom(int prg_bancos_qtd, int chr_bancos_qtd, 
               vector<byte>& arquivo, ArquivoFormato formato,
               byte espelhamento):
        Cartucho(prg_bancos_qtd, chr_bancos_qtd, arquivo, formato, espelhamento)
    {
        // aloca a memória que representará a ram PRG
        this->ram_prg.resize(0x2000);

        if (this->possui_chr_ram)
        {
            // aloca a memória que representará a ram CHR
            this->ram_chr.resize(0x2000);
        }
    }

    uint8_t NRom::ler(uint16 endereco)
    {
        if (endereco < 0x2000)
        {
            // ler a rom CHR ou a ram CHR
            if (this->possui_chr_ram)
            {
                return ram_chr.at(endereco);
            }
            else
            {
                return rom_chr.at(endereco);   
            }
        }
        else if (endereco >= 0x8000)
        {
            // os bancos da rom PRG começam a partir do endereço 0x8000
            uint16_t endereco_mapeado = endereco - 0x8000;

            // espelhar o endereço caso a rom PRG só possua 1 banco
            if (this->prg_bancos_quantidade == 1)
            {
                return this->rom_prg.at(endereco_mapeado % 0x4000);
            }
            else
            {
                return this->rom_prg.at(endereco_mapeado);
            }
        }

        return 0;
    }

    void NRom::escrever(uint16 endereco, byte valor)
    {
        if (!this->possui_chr_ram)
        {
            throw runtime_error("CHR RAM inexistente"s);
        }

        if (endereco < 0x2000)
        {
            // escrever na rom CHR
            this->rom_chr.at(endereco) = valor;
        }
    }

    string NRom::get_nome()
    {
        return "NROM";
    }
}
