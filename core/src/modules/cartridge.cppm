module;
#include <stdexcept>
#include <sstream>
#include <memory>
#include <string>
#include <vector>
export module nesbrasa.cartridge;
import nesbrasa.util;
import nesbrasa.types;
import nesbrasa.ports;

export namespace nesbrasa::nucleo::mapeadores
{
    enum class CartuchoTipo { NROM = 0, MMC1 = 1, DESCONHECIDO };
    enum class ArquivoFormato { DESCONHECIDO, INES, NES_2_0 };

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
        tipos::byte get_espelhamento() const override { return this->Cartucho::espelhamento; }
    };

    using CartuchoCriador = std::unique_ptr<Cartucho>(*)(CartuchoTipo, int, int,
                                                         std::vector<tipos::byte>&,
                                                         ArquivoFormato, tipos::byte);
    inline CartuchoCriador criador_nrom = nullptr;

    // Cartucho::criar owns the factory API, while NRom owns the concrete type.
    // Registration avoids importing mapper.nrom here and breaking the cycle.
    void registrar_criador_nrom(CartuchoCriador criador)
    {
        criador_nrom = criador;
    }

    std::unique_ptr<Cartucho> Cartucho::criar(CartuchoTipo tipo, int prg_qtd, int chr_qtd,
                                              std::vector<tipos::byte>& arquivo,
                                              ArquivoFormato formato, tipos::byte espelhamento)
    {
        if (tipo == CartuchoTipo::NROM && criador_nrom != nullptr)
        {
            return criador_nrom(tipo, prg_qtd, chr_qtd, arquivo, formato, espelhamento);
        }

        std::stringstream erro_ss;
        erro_ss << "Erro: mapeador não reconhecido\n";
        erro_ss << "Código do mapeador: " << static_cast<int>(tipo);
        throw std::runtime_error(erro_ss.str());
    }
}


using namespace nesbrasa::tipos;
using std::unique_ptr;
using std::vector;

namespace nesbrasa::nucleo::mapeadores
{
    using std::runtime_error;
    using namespace std::string_literals;

    Cartucho::Cartucho(int prg_bancos_qtd, 
                 int chr_bancos_qtd, 
                 vector<byte>& arquivo,
                 ArquivoFormato formato,
                 byte espelhamento)
    {
        this->espelhamento = espelhamento;
        this->arquivo_formato = formato;
        this->prg_bancos_quantidade = prg_bancos_qtd;
        this->chr_bancos_quantidade = chr_bancos_qtd;

        this->possui_prg_ram = buscar_bit(arquivo.at(6), 1);

        this->possui_chr_ram = false;
        if (chr_bancos_qtd == 0)
        {
            this->possui_chr_ram = true;
        }

        uint rom_prg_tamanho = prg_bancos_qtd * this->PRG_BANCOS_TAMANHO;
        uint rom_chr_tamanho = chr_bancos_qtd * this->CHR_BANCOS_TAMANHO;

        // aloca a memória necessária
        this->rom_prg.resize(rom_prg_tamanho);
        this->rom_chr.resize(rom_chr_tamanho);

        // busca o inicio da ROM PRG
        int rom_prg_inicio = 0;
        if (buscar_bit(arquivo.at(6), 2) == true)
        {
            rom_prg_inicio = 16 + 512;
        }
        else
        {
            rom_prg_inicio = 16;
        }

        // checa o tamanho do arquivo
        if ((rom_prg_inicio + rom_prg_tamanho + rom_chr_tamanho) > arquivo.size())
        {
            // formato inválido
            throw runtime_error("Erro: formato não reconhecido"s);
        }

        // calcula o inicio da ROM CHR
        int rom_chr_inicio = rom_prg_inicio + rom_prg_tamanho;

        // Copia os dados referentes à ROM PRG do arquivo para o array
        for (uint i = 0; i < this->rom_prg.size(); i++)
        {
            this->rom_prg.at(i) = arquivo.at(rom_prg_inicio+i);
        }

        // Copia os dados referentes à ROM CHR do arquivo para o array
        for (uint i = 0; i < this->rom_chr.size(); i++) 
        {
            this->rom_chr.at(i) = arquivo.at(rom_chr_inicio+i);
        }
    }

    int Cartucho::get_prg_bancos_quantidade()
    {
        return this->prg_bancos_quantidade;
    }

    int Cartucho::get_chr_bancos_quantidade()
    {
        return this->chr_bancos_quantidade;
    }
}
