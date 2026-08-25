module;
#include <memory>
#include <string>
#include <vector>
export module nesbrasa.cartridge;
import nesbrasa.types;

export namespace nesbrasa::nucleo::mapeadores
{
    enum class CartuchoTipo { NROM = 0, MMC1 = 1, DESCONHECIDO };
    enum class ArquivoFormato { DESCONHECIDO, INES, NES_2_0 };

    class Cartucho
    {
    protected:
        int prg_bancos_quantidade, chr_bancos_quantidade;
        bool possui_prg_ram, possui_chr_ram;
        std::vector<tipos::byte> rom_prg, rom_chr, ram_prg, ram_chr;
    public:
        ArquivoFormato arquivo_formato;
        tipos::byte espelhamento;
        static const int PRG_BANCOS_TAMANHO, CHR_BANCOS_TAMANHO;
        static std::unique_ptr<Cartucho> criar(CartuchoTipo, int, int, std::vector<tipos::byte>&, ArquivoFormato, tipos::byte);
        Cartucho(int, int, std::vector<tipos::byte>&, ArquivoFormato, tipos::byte);
        virtual ~Cartucho() = default;
        virtual tipos::byte ler(tipos::uint16) = 0;
        virtual void escrever(tipos::uint16, tipos::byte) = 0;
        virtual std::string get_nome() = 0;
        int get_prg_bancos_quantidade();
        int get_chr_bancos_quantidade();
    };
}
