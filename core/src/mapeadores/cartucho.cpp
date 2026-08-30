#include <nesbrasa/mapeadores/cartucho.hpp>
#include <nesbrasa/util.hpp>

namespace nesbrasa::nucleo::mapeadores
{
CartuchoCriador criador_nrom = nullptr;

void registrar_criador_nrom(CartuchoCriador criador)
{
    criador_nrom = criador;
}

std::unique_ptr<Cartucho> Cartucho::criar(CartuchoTipo tipo, int prg_qtd, int chr_qtd,
                                                std::vector<tipos::byte>& arquivo,
                                                ArquivoFormato formato, tipos::byte espelhamento) {

            if (tipo == CartuchoTipo::NROM && criador_nrom != nullptr)
            {
                return criador_nrom(tipo, prg_qtd, chr_qtd, arquivo, formato, espelhamento);
            }

            std::stringstream erro_ss;
            erro_ss << "Erro: mapeador não reconhecido\n";
            erro_ss << "Código do mapeador: " << static_cast<int>(tipo);
            throw std::runtime_error(erro_ss.str());
        
}

Cartucho::Cartucho(int prg_bancos_qtd, int chr_bancos_qtd,
                 std::vector<tipos::byte>& arquivo, ArquivoFormato formato,
                 tipos::byte espelhamento) {

            this->espelhamento = espelhamento;
            this->arquivo_formato = formato;
            this->prg_bancos_quantidade = prg_bancos_qtd;
            this->chr_bancos_quantidade = chr_bancos_qtd;

            this->possui_prg_ram = buscar_bit(arquivo.at(6), 1);
            this->possui_chr_ram = chr_bancos_qtd == 0;

            tipos::uint rom_prg_tamanho = prg_bancos_qtd * this->PRG_BANCOS_TAMANHO;
            tipos::uint rom_chr_tamanho = chr_bancos_qtd * this->CHR_BANCOS_TAMANHO;

            // aloca a memória necessária
            this->rom_prg.resize(rom_prg_tamanho);
            this->rom_chr.resize(rom_chr_tamanho);

            // busca o inicio da ROM PRG
            int rom_prg_inicio = buscar_bit(arquivo.at(6), 2) ? 16 + 512 : 16;

            // checa o tamanho do arquivo
            if ((rom_prg_inicio + rom_prg_tamanho + rom_chr_tamanho) > arquivo.size())
            {
                throw std::runtime_error("Erro: formato não reconhecido");
            }

            // calcula o inicio da ROM CHR
            int rom_chr_inicio = rom_prg_inicio + rom_prg_tamanho;

            // Copia os dados referentes à ROM PRG do arquivo para o array
            for (tipos::uint i = 0; i < this->rom_prg.size(); i++)
            {
                this->rom_prg.at(i) = arquivo.at(rom_prg_inicio + i);
            }

            // Copia os dados referentes à ROM CHR do arquivo para o array
            for (tipos::uint i = 0; i < this->rom_chr.size(); i++)
            {
                this->rom_chr.at(i) = arquivo.at(rom_chr_inicio + i);
            }
        
}

int Cartucho::get_prg_bancos_quantidade() {
 return this->prg_bancos_quantidade; 
}

int Cartucho::get_chr_bancos_quantidade() {
 return this->chr_bancos_quantidade; 
}

tipos::byte Cartucho::get_espelhamento() const {
 return this->Cartucho::espelhamento; 
}
}
