module;
#include <string>
#include <sstream>
#include <iostream>
#include <array>
#include <memory>
#include <vector>
export module nesbrasa.nes;
import nesbrasa.util;
import nesbrasa.mapper.nrom;
export import nesbrasa.types;
export import nesbrasa.memory;
export import nesbrasa.cpu;
export import nesbrasa.ppu;
export import nesbrasa.controller;
export import nesbrasa.cartridge;
import nesbrasa.ports;

export namespace nesbrasa::nucleo
{
    class Nes : public InterruptSink
    {
    public:
        static constexpr int TELA_LARGURA = 256;
        static constexpr int TELA_ALTURA = 240;
        static constexpr int CICLOS_POR_QUADRO = 29780;
        Memoria memoria;
        Cpu cpu;
        Ppu ppu;
        Controle controle_1, controle_2;
        std::unique_ptr<mapeadores::Cartucho> cartucho;
        bool is_programa_carregado;
        Nes();
        void carregar_rom(std::vector<tipos::byte> arquivo);
        int avancar();
        int avancar_quadro();
        const std::array<tipos::uint32, 256 * 240>& get_textura() const;
        void set_botao(Botao botao, bool pressionado);
        bool programa_carregado() const;
        void ativar_interrupcao(Interrupcao) override;
    };
}


using namespace nesbrasa::tipos;

namespace nesbrasa::nucleo
{
    using std::make_unique;
    using std::stringstream;
    using std::runtime_error;
    using std::array;
    using std::vector;
    using std::string;
    using namespace std::string_literals;
    using namespace mapeadores;

    Nes::Nes(): 
        memoria(),
        cpu(&this->memoria),
        ppu(&this->memoria, nullptr, this, &this->cpu)
    {
        mapeadores::registrar_nrom();
        this->memoria.configurar(&this->ppu, &this->controle_1, &this->controle_2, nullptr, this);
        this->is_programa_carregado = false;
        this->cartucho = nullptr;
        this->memoria.configurar(&this->ppu, &this->controle_1, &this->controle_2, nullptr, this);
        this->ppu.configurar_cartucho(nullptr);
    }

    void Nes::carregar_rom(vector<byte> arquivo)
    {
        this->cartucho = nullptr;
        this->is_programa_carregado = false;
        auto formato = ArquivoFormato::DESCONHECIDO;

        // checa se o arquivo é grande o suficiente para ter um cabeçalho
        if (arquivo.size() < 16)
        {
            throw runtime_error("Erro: formato não reconhecido"s);
        }

        // lê os 4 primeiros bytes do arquivo como uma string
        string formato_string;
        for (int i = 0; i < 4; i++)
        {
            formato_string += static_cast<char>(arquivo.at(i));
        }

        // arquivos nos formatos iNES e NES 2.0 começam com a string "NES\x1A"
        if (formato_string == "NES\x1A")
        {
            if ((buscar_bit(arquivo.at(7), 2) == false) && (buscar_bit(arquivo.at(7), 3) == true))
            {
                // o arquivo está no formato NES 2.0
                formato = ArquivoFormato::NES_2_0;
            }
            else
            {
                // o arquivo está no formato iNES
                formato = ArquivoFormato::INES;
            }
        }
        else
        {
            // formato inválido
            throw runtime_error("Erro: formato não reconhecido"s);
        }

        int prg_qtd = arquivo.at(4); // quantidade de bancos na rom prg
        int chr_qtd = arquivo.at(5); // quantidade de bancos na rom chr

        byte mapeador_nibble_menor = (arquivo.at(6) & 0xF0) >> 4;
        byte mapeador_nibble_maior = (arquivo.at(7) & 0xF0) >> 4;
        byte mapeador_codigo = (mapeador_nibble_maior << 4) | mapeador_nibble_menor;

        byte espelhamento1 = buscar_bit(arquivo.at(6), 0);
        byte espelhamento2 = buscar_bit(arquivo.at(6), 3);
        byte espelhamento = espelhamento1 | espelhamento2 << 1;

        // transforma o valor numérico em um enum
        auto cartucho_tipo = static_cast<CartuchoTipo>(mapeador_codigo);
        // Usar o método factory da classe Cartucho para criar o objeto do cartucho
        this->cartucho = Cartucho::criar(cartucho_tipo, prg_qtd, chr_qtd, arquivo, formato, espelhamento);
        this->memoria.configurar(&this->ppu, &this->controle_1, &this->controle_2, this->cartucho.get(), this);
        this->ppu.configurar_cartucho(this->cartucho.get());

        //TODO: Completar suporte a ROMs no formato NES 2.0
        this->is_programa_carregado = true;
        this->cpu.resetar();
    }

    int Nes::avancar()
    {
        if (!this->is_programa_carregado)
        {
            throw runtime_error("Erro: nenhum programa na memória"s);
        }

        const int cpu_ciclos = this->cpu.avancar();
        const int ppu_ciclos = cpu_ciclos * 3;
        for (int i = 0; i < ppu_ciclos; i++)
        {
            this->ppu.avancar();
        }

        return cpu_ciclos;
    }

    int Nes::avancar_quadro()
    {
        int ciclos = 0;
        while (ciclos < CICLOS_POR_QUADRO)
        {
            ciclos += this->avancar();
        }
        return ciclos;
    }

    const array<uint32, (256 * 240)>& Nes::get_textura() const
    {
        return this->ppu.get_textura();
    }

    void Nes::set_botao(Botao botao, bool pressionado)
    {
        this->controle_1.set_valor(botao, pressionado);
    }

    bool Nes::programa_carregado() const
    {
        return this->is_programa_carregado;
    }

    void Nes::ativar_interrupcao(Interrupcao interrupcao)
    {
        this->cpu.interrupcao = interrupcao;
    }
}
