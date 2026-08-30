#include <nesbrasa/mapeadores/nrom.hpp>

namespace nesbrasa::nucleo::mapeadores
{
std::unique_ptr<Cartucho> criar_nrom(CartuchoTipo, int prg_qtd, int chr_qtd,
                                     std::vector<tipos::byte>& arquivo,
                                     ArquivoFormato formato, tipos::byte espelhamento)
{
    return std::make_unique<NRom>(prg_qtd, chr_qtd, arquivo, formato, espelhamento);
}

void registrar_nrom()
{
    registrar_criador_nrom(&criar_nrom);
}

NRom::NRom(int prg_bancos_qtd, int chr_bancos_qtd,
             std::vector<tipos::byte>& arquivo, ArquivoFormato formato,
             tipos::byte espelhamento):
            Cartucho(prg_bancos_qtd, chr_bancos_qtd, arquivo, formato, espelhamento) {

            // aloca a memória que representará a ram PRG
            this->ram_prg.resize(0x2000);

            if (this->possui_chr_ram)
            {
                // aloca a memória que representará a ram CHR
                this->ram_chr.resize(0x2000);
            }
        
}

tipos::byte NRom::ler(tipos::uint16 endereco) {

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
                tipos::uint16 endereco_mapeado = endereco - 0x8000;

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

void NRom::escrever(tipos::uint16 endereco, tipos::byte valor) {

            if (!this->possui_chr_ram)
            {
                throw std::runtime_error("CHR RAM inexistente");
            }

            if (endereco < 0x2000)
            {
                // escrever na rom CHR
                this->rom_chr.at(endereco) = valor;
            }
        
}

std::string NRom::get_nome() {

            return "NROM";
        
}
}
