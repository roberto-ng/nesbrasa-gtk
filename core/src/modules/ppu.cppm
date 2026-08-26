module;
#include <iostream>
#include <array>
#include <memory>
export module nesbrasa.ppu;
import nesbrasa.cores;
import nesbrasa.types;
import nesbrasa.ports;

export namespace nesbrasa::nucleo
{
    extern std::array<std::array<tipos::uint16, 4>, 5> espelhamento_tabela;

    class Ppu : public PpuPort
    {
        using Framebuffer = std::array<tipos::uint32, 256 * 240>;
        CpuBus* memoria;
        CartridgePort* cartucho;
        InterruptSink* interrupcoes;
        DmaSink* dma;
        int ciclo, scanline;
        tipos::uint64 frame;
        std::array<tipos::byte, 0x20> paletas;
        std::array<tipos::byte, 0x800> tabelas_de_nomes;
        std::array<tipos::byte, 0x100> oam;
        std::unique_ptr<Framebuffer> frente, fundo;
        tipos::uint16 v, t;
        tipos::byte x, ultimo_valor, nmi_atrasar, tabela_de_nomes_byte, tabela_de_atributos_byte, tile_byte_maior, tile_byte_menor;
        bool w, f, nmi_ocorreu, nmi_output, nmi_anterior;
        tipos::uint64 tile_dados;
        int sprites_qtd;
        std::array<tipos::uint64, 8> sprites_padroes;
        std::array<tipos::byte, 8> sprites_posicoes, sprites_prioridades;
        std::array<int, 8> sprites_indices;
        tipos::uint16 vram_incrementar, sprite_padrao_tabela_endereco;
        tipos::byte flag_nametable_base, oam_endereco, buffer_dados;
        bool flag_incrementar, flag_padrao_sprite, flag_padrao_fundo, flag_sprite_altura, flag_mestre_escravo;
        bool flag_enfase_b, flag_enfase_g, flag_enfase_r, flag_sprite_habilitar, flag_fundo_habilitar;
        bool flag_sprite_habilitar_col_esquerda, flag_fundo_habilitar_col_esquerda, flag_escala_cinza;
        bool flag_sprite_zero, flag_sprite_transbordamento;
    public:
        Ppu(CpuBus*, CartridgePort*, InterruptSink*, DmaSink*);
        void configurar_cartucho(CartridgePort*);
        void reiniciar(); void atualizar(); void avancar();
        tipos::byte ler(tipos::uint16); void escrever(tipos::uint16, tipos::byte);
        tipos::byte registrador_ler(tipos::uint16); void registrador_escrever(tipos::uint16, tipos::byte);
        tipos::byte ler_paleta(tipos::uint16); void escrever_paleta(tipos::uint16, tipos::byte);
        std::array<tipos::uint32, 256 * 240>& get_textura();
        const std::array<tipos::uint32, 256 * 240>& get_textura() const;
    private:
        tipos::byte buscar_pixel_fundo(); tipos::byte buscar_pixel_sprite(tipos::byte&); tipos::byte buscar_cor_fundo(tipos::byte); tipos::byte buscar_cor_pixel(tipos::byte);
        tipos::uint32 buscar_padrao_sprite(int, int); void renderizar_pixel(); void executar_ciclo_vblank(); void encerrar_ciclo_vblank(); void alterar_nmi();
        void buscar_byte_tabela_de_nomes(); void buscar_byte_tabela_de_atributos(); void buscar_tile_byte_menor(); void buscar_tile_byte_maior(); void tile_guardar_dados(); void avaliar_sprites();
        void copiar_x(); void copiar_y(); void mudar_scroll_x(); void mudar_scroll_y(); void set_controle(tipos::byte); void set_mascara(tipos::byte); tipos::byte get_estado(); void set_oam_enderco(tipos::byte); void set_oam_dados(tipos::byte); tipos::byte get_oam_dados(); void set_scroll(tipos::byte); void set_endereco(tipos::byte); void set_omd_dma(tipos::byte); tipos::byte get_dados(); void set_dados(tipos::byte); tipos::uint16 endereco_espelhado(tipos::byte, tipos::uint16); void set_textura_valor(std::array<tipos::byte, 256 * 240>&, int, int, int);
    };
}


using namespace nesbrasa::tipos;
using std::array;

namespace nesbrasa::nucleo
{
    const int TELA_LARGURA = 256;
    const int TELA_ALTURA = 240;

    array< array<uint16, 4>, 5> espelhamento_tabela {
        array<uint16, 4> {0, 0, 1, 1},
        array<uint16, 4> {0, 1, 0, 1},
        array<uint16, 4> {0, 0, 0, 0},
        array<uint16, 4> {1, 1, 1, 1},
        array<uint16, 4> {0, 1, 2, 3},
    };

    Ppu::Ppu(CpuBus* memoria, CartridgePort* cartucho, InterruptSink* interrupcoes, DmaSink* dma):
        memoria(memoria),
        cartucho(cartucho), interrupcoes(interrupcoes), dma(dma),
        frente(std::make_unique<Framebuffer>()),
        fundo(std::make_unique<Framebuffer>())
    {
        this->ciclo = 0;
        this->scanline = 261;
        this->frame = 0;

        this->buffer_dados = 0;
        this->ultimo_valor = 0;
        this->vram_incrementar = 0;
        this->oam_endereco = 0;
        // Keep unused sprites off-screen until the game populates OAM.
        this->oam.fill(0xFF);

        this->tile_dados = 0;
        this->tile_byte_maior = 0;
        this->tile_byte_menor = 0;
        this->tabela_de_nomes_byte = 0;
        this->tabela_de_atributos_byte = 0;

        this->sprites_qtd = 0;

        this->nmi_ocorreu = false;
        this->nmi_anterior = false;
        this->nmi_atrasar = false;
        this->nmi_output = false;

        this->flag_mestre_escravo = false;
        this->flag_sprite_altura = false;
        this->flag_padrao_fundo = false;
        this->flag_padrao_sprite = false;
        this->flag_incrementar = false;
        this->flag_nametable_base = 0;

        this->flag_enfase_b = false;
        this->flag_enfase_g = false;
        this->flag_enfase_r = false;
        this->flag_sprite_habilitar = false;
        this->flag_fundo_habilitar = false;
        this->flag_sprite_habilitar_col_esquerda = false;
        this->flag_fundo_habilitar_col_esquerda = false;
        this->flag_escala_cinza = false;

        this->flag_sprite_zero = false;
        this->flag_sprite_transbordamento = false;

        this->v = 0;
        this->t = 0;
        this->x = 0;
        this->w = false;
        this->f = false;

        this->reiniciar();
    }

    void Ppu::configurar_cartucho(CartridgePort* cartucho)
    {
        this->cartucho = cartucho;
    }

    void Ppu::reiniciar()
    {
        this->frame = 0;
        this->ciclo = 340;
        this->scanline = 240;

        this->set_oam_enderco(0);
        this->set_controle(0);
        this->set_mascara(0);        
    }

    void Ppu::atualizar()
    {
        if (nmi_atrasar > 0)
        {
            nmi_atrasar -= 1;
            if (nmi_atrasar == 0 && this->nmi_output && this->nmi_ocorreu)
            {
                this->interrupcoes->ativar_interrupcao(Interrupcao::NMI);
            }
        }

        if (this->flag_fundo_habilitar != 0 || this->flag_sprite_habilitar != 0)
        {
            if (this->f == 1 && this->scanline == 261 && this->ciclo == 339)
            {
                this->ciclo = 0;
                this->scanline = 0;
                this->frame += 1;
                this->f ^= 1;
                return;
            }
        }

        this->ciclo += 1;
        if (this->ciclo > 340)
        {
            this->ciclo = 0;
            this->scanline += 1;

            if (this->scanline > 261)
            {
                this->scanline = 0;
                this->frame += 1;
                this->f ^= 1;
            }
        }
    }

    void Ppu::avancar()
    {
        this->atualizar();

        bool renderizacao_habilitada = this->flag_fundo_habilitar || this->flag_sprite_habilitar;
        bool prelinha = this->scanline == 261;
        bool linha_visivel = this->scanline < 240;
        bool linha_renderizacao = prelinha || linha_visivel;
        bool ciclo_pre_busca = this->ciclo >= 321 && this->ciclo <= 336;
        bool ciclo_visivel = this->ciclo >= 1 && this->ciclo <= 256;
        bool ciclo_busca = ciclo_pre_busca || ciclo_visivel;
        
        // se a renderização estiver habilitada
        if (renderizacao_habilitada)
        {
            if (linha_visivel && ciclo_visivel)
            {
                this->renderizar_pixel();
            }

            if (linha_renderizacao && ciclo_busca)
            {
                this->tile_dados <<= 4;

                switch (this->ciclo % 8)
                {
                    case 1:
                        // buscar byte da tabela de nomes
                        this->buscar_byte_tabela_de_nomes();
                        break;

                    case 3:
                        // buscar byte da tabela de atributos
                        this->buscar_byte_tabela_de_atributos();
                        break;

                    case 5:
                        // buscar o byte de menor significancia do tile
                        this->buscar_tile_byte_menor();
                        break;
                        
                    case 7:
                        // buscar o byte de maior significancia do tile
                        this->buscar_tile_byte_maior();
                        break;
                        
                    case 0:
                        // guardar os dados do tile
                        this->tile_guardar_dados();
                        break;

                    default:
                        break;
                }
            }

            if (prelinha && this->ciclo >= 280 && this->ciclo <= 304)
            {
                this->copiar_y();
            }

            if (linha_renderizacao)
            {
                if (ciclo_busca && this->ciclo%8 == 0)
                {
                    this->mudar_scroll_x();
                }

                if (this->ciclo == 256)
                    this->mudar_scroll_y();

                if (this->ciclo == 257)
                    this->copiar_x();
            }
        }

        if (renderizacao_habilitada)
        {
            if (this->ciclo == 257)
            {
                if (linha_visivel)
                {
                    // avaliar sprites
                    this->avaliar_sprites();
                }
                else
                {
                    this->sprites_qtd = 0;
                }
            }
        }

        if (this->scanline == 241 && this->ciclo == 1)
        {
            this->executar_ciclo_vblank();
        }
        if (prelinha && this->ciclo == 1)
        {
            this->encerrar_ciclo_vblank();
            this->flag_sprite_zero = false;
            this->flag_sprite_transbordamento = false;
        }
    }

    byte Ppu::registrador_ler(uint16 endereco)
    {
        switch (endereco)
        {
            case 0x2002:
                return this->get_estado();

            case 0x2004:
                return this->get_oam_dados();

            case 0x2007:
                return this->get_dados();

            default:
                return 0;
        }
    }

    void Ppu::registrador_escrever(uint16 endereco, byte valor)
    {
        this->ultimo_valor = valor;
        switch (endereco)
        {
            case 0x2000:
                this->set_controle(valor);
                break;

            case 0x2001:
                this->set_mascara(valor);
                break;

            case 0x2003:
                this->set_oam_enderco(valor);
                break;

            case 0x2005:
                this->set_scroll(valor);
                break;

            case 0x2006:
                this->set_endereco(valor);
                break;

            case 0x2007:
                this->set_dados(valor);
                break;
            
            case 0x4014:
                this->set_omd_dma(valor);
                break;

            default:
                break;
        }
    }

    byte Ppu::ler(uint16 endereco)
    {
        endereco = endereco % 0x4000;

        if (endereco < 0x2000)
        {
            return this->cartucho->ler(endereco);
        }
        else if (endereco >= 0x2000 && endereco < 0x3F00)
        {
            byte modo = this->cartucho->get_espelhamento();
            uint16 endereco_espelhado = this->endereco_espelhado(modo, endereco);
            uint16 posicao = endereco_espelhado % this->tabelas_de_nomes.size();

            return this->tabelas_de_nomes.at(posicao);
        }
        else if (endereco >= 0x3F00 && endereco < 0x4000)
        {
            // ler dados das paletas de cores
            return this->ler_paleta(endereco % this->paletas.size());
        }
        else
        {
            throw std::runtime_error("Escrita em endereço desconhecido da PPU");
        }

        return 0;
    }

    void Ppu::escrever(uint16 endereco, byte valor)
    {
        endereco = endereco % 0x4000;
        if (endereco < 0x2000)
        {
            this->cartucho->escrever(endereco, valor);
        }
        else if (endereco >= 0x2000 && endereco < 0x3F00)
        {
            byte modo = this->cartucho->get_espelhamento();
            uint16 endereco_espelhado = this->endereco_espelhado(modo, endereco);
            uint16 posicao = endereco_espelhado % this->tabelas_de_nomes.size();
            
            this->tabelas_de_nomes.at(posicao) = valor;
        }
        else if (endereco >= 0x3F00 && endereco < 0x4000)
        {
            //escrever nas paletas de cores
            this->escrever_paleta(endereco % this->paletas.size(), valor);
        }
    }

    byte Ppu::ler_paleta(uint16 endereco)
    {
        if (endereco >= 16 && endereco%4 == 0)
		    endereco -= 16;

        return this->paletas.at(endereco);
    }

    void Ppu::escrever_paleta(uint16 endereco, byte valor)
    {
        if (endereco >= 16 && endereco%4 == 0)
            endereco -= 16;

        this->paletas.at(endereco) = valor;
    }

    byte Ppu::buscar_pixel_fundo()
    {
        if (!this->flag_fundo_habilitar)
            return 0;
        
        uint32 tile = static_cast<uint32>(this->tile_dados >> 32);
        uint32 cor = tile >> ((7 - this->x) * 4);
        return static_cast<byte>(cor & 0x0F);
    }

    byte Ppu::buscar_pixel_sprite(byte& indice)
    {
        if (this->flag_sprite_habilitar == 0)
        {
            indice = 0;
            return 0;
        }

        for (int i = 0; i < this->sprites_qtd; i++)
        {
            int offset = (this->ciclo - 1) - static_cast<int>(this->sprites_posicoes.at(i));
            if (offset < 0 || offset > 7) 
            {
                continue;
            }

            offset = 7 - offset;
            byte cor = (this->sprites_padroes.at(i) >> (offset*4)) & 0x0F;
            if (cor%4 == 0) 
            {
                continue;
            }

            indice = i;
            return cor;
        }

        indice = 0;
        return 0;
    }

    byte Ppu::buscar_cor_fundo(byte dados)
    {
        int cor_num = dados & 0x3;
        int paleta_num = (dados >> 2) & 0x3;

        if (cor_num == 0) return this->ler(0x3F00);

        uint16 paleta_endereco = 0;
        switch (paleta_num)
        {
            case 0:
                paleta_endereco = 0x3F01;
                break;

            case 1:
                paleta_endereco = 0x3F05;
                break;
            
            case 2:
                paleta_endereco = 0x3F09;
                break;
            
            case 3:
                paleta_endereco = 0x3F0D;
                break;
            
            default: break;
        }

        paleta_endereco += cor_num - 1;
        return this->ler(paleta_endereco);
    }

    byte Ppu::buscar_cor_pixel(byte dados)
    {
        int cor_num = dados & 0x3;
        int paleta_num = (dados >> 2) & 0x3;

        if (cor_num == 0) return this->ler(0x3F00);

        uint16 paleta_endereco = 0;
        switch (paleta_num)
        {
            case 0:
                paleta_endereco = 0x3F11;
                break;
            
            case 1:
                paleta_endereco = 0x3F15;
                break;

            case 2:
                paleta_endereco = 0x3F19;
                break;

            case 3:
                paleta_endereco = 0x3F1D;
                break;
            
            default: break;
        }

        paleta_endereco += cor_num - 1;
        return this->ler(paleta_endereco);
    }

    uint32 Ppu::buscar_padrao_sprite(int i, int linha)
    {
        uint16 tile = this->oam.at(i*4+1);
        byte atributos = this->oam.at(i*4+2);
        
        uint16 endereco = 0;
        if (!this->flag_sprite_altura)
        {
            if ((atributos & 0x80) == 0x80)
            {
                linha = 7 - linha;
            }

            uint16 tabela = this->flag_padrao_sprite ? 1 : 0;
            endereco = 0x1000*tabela + tile*16 + linha;
        }
        else
        {
            if ((atributos & 0x80) == 0x80)
            {
                linha = 15 - linha;
            }

            uint16 tabela = tile & 1;
            tile &= 0xFE;
            if (linha > 7)
            {
                tile += 1;
                linha -= 8;
            }
            endereco = 0x1000*tabela + tile*16 + linha;
        }

        byte atrib = (atributos & 3) << 2;
        byte tile_byte_menor = this->ler(endereco);
        byte tile_byte_maior = this->ler(endereco+8);

        uint32 valor = 0;
        for (int i = 0; i < 8; i++)
        {
            byte p1 = 0;
            byte p2 = 0;
            if ((atributos & 0x40) == 0x40)
            {
                p1 = (tile_byte_menor & 1) << 0;
                p2 = (tile_byte_maior & 1) << 1;
                tile_byte_menor >>= 1;
                tile_byte_maior >>= 1;
            }
            else
            {
                p1 = (tile_byte_menor & 0x80) >> 7;
                p2 = (tile_byte_maior & 0x80) >> 6;
                tile_byte_menor <<= 1;
                tile_byte_maior <<= 1;
            }

            valor <<= 4;
            valor |= static_cast<uint32>(atrib | p1 | p2);
        }

        return valor;
    }

    void Ppu::renderizar_pixel()
    {
        int pos_x = this->ciclo - 1;
        int pos_y = this->scanline;
        byte indice = 0;
        byte fundo = this->buscar_pixel_fundo();
        byte sprite = this->buscar_pixel_sprite(indice);

        if (pos_x < 8 && !this->flag_fundo_habilitar_col_esquerda)
            fundo = 0;
        if (pos_x < 8 && !this->flag_sprite_habilitar_col_esquerda)
            sprite = 0;

        bool f = fundo%4 != 0;
        bool s = sprite%4 != 0;
        byte cor = 0;
        if (!f && !s)
        {
            cor = 0;
        }
        else if (!f && s)
        {
            cor = sprite | 0x10;
        }
        else if (f && !s)
        {
            cor = fundo;
        }
        else
        {
            if (this->sprites_indices.at(indice) == 0 && pos_x < 255)
            {
                this->flag_sprite_zero = true;
            }
            
            if (this->sprites_prioridades.at(indice) == 0)
            {
                cor = sprite | 0x10;
            }
            else
            {
                cor = fundo;
            }
        }
        
        auto cor_nes = this->ler_paleta(static_cast<uint16>(cor));
        this->fundo->at(pos_y*256 + pos_x) = cores::tabela_rgb.at(cor_nes%64);
    }

    void Ppu::executar_ciclo_vblank()
    {
        // Swap the heap-owned fixed-size framebuffers without copying them.
        this->frente.swap(this->fundo);

        this->nmi_ocorreu = true;
        this->alterar_nmi();
    }

    void Ppu::encerrar_ciclo_vblank()
    {
        this->nmi_ocorreu = false;
        this->alterar_nmi();
    }

    void Ppu::alterar_nmi()
    {
        bool nmi = this->nmi_output && this->nmi_ocorreu;
        if (nmi && !this->nmi_anterior)
        {
            this->nmi_atrasar = 15;
        }
        this->nmi_anterior = nmi;
    }

    void Ppu::buscar_byte_tabela_de_nomes()
    {
        uint16 v = this->v;
	    uint16 endereco = 0x2000 | (v & 0x0FFF);
	    this->tabela_de_nomes_byte = this->ler(endereco);
    }

    void Ppu::buscar_byte_tabela_de_atributos()
    {
        uint16 v = this->v;
        uint16 endereco = 0x23C0 | (v & 0x0C00) | ((v >> 4) & 0x38) | ((v >> 2) & 0x07);
        uint16 shift = ((v >> 4) & 4) | (v & 2);
        this->tabela_de_atributos_byte = ((this->ler(endereco) >> shift) & 3) << 2;
    }

    void Ppu::buscar_tile_byte_menor()
    {
        uint16 y = (this->v >> 12) & 7;
        uint16 tabela = this->flag_padrao_fundo ? 1 : 0;
        uint16 tile = this->tabela_de_nomes_byte;
        uint16 endereco = 0x1000*tabela + tile*16 + y;
        this->tile_byte_menor = this->ler(endereco);
    }

    void Ppu::buscar_tile_byte_maior()
    {
        uint16 y = (this->v >> 12) & 7;
        uint16 tabela = this->flag_padrao_fundo ? 1 : 0;
        uint16 tile = this->tabela_de_nomes_byte;
        uint16 endereco = 0x1000*tabela + tile*16 + y;
        this->tile_byte_maior = this->ler(endereco+8);
    }

    void Ppu::tile_guardar_dados()
    {
        uint32 valor = 0;
        for (int i = 0; i < 8; i++)
        {
            byte a = this->tabela_de_atributos_byte;
            byte p1 = (this->tile_byte_menor & 0x80) >> 7;
            byte p2 = (this->tile_byte_maior & 0x80) >> 6;
            this->tile_byte_menor <<= 1;
		    this->tile_byte_maior <<= 1;
            valor <<= 4;
            valor |= static_cast<uint32>(a | p1 | p2);
        }
        this->tile_dados |= static_cast<uint64>(valor);
    }

    void Ppu::avaliar_sprites()
    {
        int altura = 0;
        if (!this->flag_sprite_altura)
        {
            altura = 8;
        }
        else
        {
            altura = 16;
        }

        int contagem = 0;
        for (int i = 0; i < 64; i++)
        {
            uint pos_y = this->oam.at(i*4+0);
            uint atrib = this->oam.at(i*4+2);
            uint pos_x = this->oam.at(i*4+3);
            int linha = this->scanline - static_cast<int>(pos_y);
            if (linha < 0 || linha >= altura)
            {
                continue;
            }

            if (contagem < 8)
            {
                this->sprites_padroes.at(contagem) = this->buscar_padrao_sprite(i, linha);
                this->sprites_posicoes.at(contagem) = pos_x;
                this->sprites_prioridades.at(contagem) = (atrib >> 5) & 1;
                this->sprites_indices.at(contagem) = i;
            }
            contagem++;
        }
        if (contagem > 8)
        {
            contagem = 8;
            this->flag_sprite_transbordamento = true;
        }
        this->sprites_qtd = contagem;
    }

    void Ppu::copiar_x()
    {
        // v: ....F.. ...EDCBA = t: ....F.. ...EDCBA
        this->v = (this->v & 0xFBE0) | (this->t & 0x041F);
    }

    void Ppu::copiar_y()
    {
        // v: IHGF.ED CBA..... = t: IHGF.ED CBA.....
        this->v = (this->v & 0x841F) | (this->t & 0x7BE0);
    }

    void Ppu::mudar_scroll_x()
    {
        if ((this->v & 0x001F) == 31)
        {
            // x = 0
            this->v &= 0xFFE0;
            this->v ^= 0x0400;
        }
        else
        {
            this->v += 1;
        }
    }

    void Ppu::mudar_scroll_y()
    {
        if ((this->v & 0x7000) != 0x7000)
        {
            this->v += 0x1000;
        }
        else
        {
            this->v &= 0x8FFF;
            uint16 y = (this->v & 0x03E0) >> 5;
            if (y == 29)
            {
                y = 0;
                this->v ^= 0x0800;
            }
            else if (y == 31)
            {
                y = 0;
            }
            else
            {
                y += 1;
            }

            this->v = (this->v & 0xFC1F) | (y << 5);
        }
    }

    void Ppu::set_controle(byte valor)
    {
        this->flag_nametable_base = (valor >> 0) & 3;
        this->flag_incrementar = (valor >> 2) & 1;
        this->flag_padrao_sprite =  (valor >> 3) & 1;
        this->flag_padrao_fundo = (valor >> 4) & 1;
        this->flag_sprite_altura = (valor >> 5) & 1;
        this->flag_mestre_escravo = (valor >> 6) & 1;
        this->nmi_output = ((valor >> 7) & 1) == 1;
        this->alterar_nmi();

        this->sprite_padrao_tabela_endereco = 0x1000 * this->flag_padrao_sprite;

        // t: ...BA.. ........ = d: ......BA
        this->t = (this->t & 0xF3FF) | ((static_cast<uint16>(valor) & 0x03) << 10);
    }

    void Ppu::set_mascara(byte valor)
    {
        this->flag_escala_cinza = (valor >> 0) & 1;
        this->flag_fundo_habilitar_col_esquerda = (valor >> 1) & 1;
        this->flag_sprite_habilitar_col_esquerda = (valor >> 2) & 1;
        this->flag_fundo_habilitar = (valor >> 3) & 1;
        this->flag_sprite_habilitar = (valor >> 4) & 1;
        this->flag_enfase_r = (valor >> 5) & 1;
        this->flag_enfase_g = (valor >> 6) & 1;
        this->flag_enfase_b = (valor >> 7) & 1;
    }

    byte Ppu::get_estado()
    {
        byte resultado = this->ultimo_valor & 0x1F;
        resultado |= static_cast<byte>(this->flag_sprite_transbordamento) << 5;
        resultado |= static_cast<byte>(this->flag_sprite_zero) << 6;
        
        if (this->nmi_ocorreu)
        {
            resultado |= 1 << 7;
        }
        this->nmi_ocorreu = false;
        this->alterar_nmi();
        
        // w: = 0
        this->w = 0;
        return resultado;
    }

    void Ppu::set_oam_enderco(byte valor)
    {
        this->oam_endereco = valor;
    }

    void Ppu::set_oam_dados(byte valor)
    {
        this->oam.at(this->oam_endereco) = valor;
        this->oam_endereco += 1;
    }

    byte Ppu::get_oam_dados()
    {
        return this->oam.at(this->oam_endereco);
    }

    void Ppu::set_scroll(byte valor)
    {
        // se o valor de 'w' for 0, estamos na primeira escrita
        // caso não seja, estamos na segunda escrita
        if (this->w == false)
        {
            // t: ....... ...HGFED = d: HGFED...
            // x:              CBA = d: .....CBA
            // w:                  = 1
            this->t = (this->t & 0xFFE0) | (static_cast<uint16>(valor) >> 3);
            this->x = valor & 0x07;
            this->w = 1;
        }
        else
        {
            // t: CBA..HG FED..... = d: HGFEDCBA
            // w:                  = 0
            this->t = (this->t & 0x8FFF) | ((static_cast<uint16>(valor) & 0x07) << 12);
		    this->t = (this->t & 0xFC1F) | ((static_cast<uint16>(valor) & 0xF8) << 2);
		    this->w = 0;
        }
    }

    void Ppu::set_endereco(byte valor)
    {
        // se o valor de 'w' for 0, estamos na primeira escrita
        // caso não seja, estamos na segunda escrita
        if (this->w == 0)
        {
            // t: .FEDCBA ........ = d: ..FEDCBA
            // t: X...... ........ = 0
            // w:                  = 1
            this->t = (this->t & 0x80FF) | ((static_cast<uint16>(valor) & 0x3F) << 8);
		    this->w = 1;
        }
        else
        {
            // t: ....... HGFEDCBA = d: HGFEDCBA
            // v                   = t
            // w:                  = 0
            this->t = (this->t & 0xFF00) | static_cast<uint16>(valor);
		    this->v = this->t;
		    this->w = 0;
        }
    }

    void Ppu::set_omd_dma(byte valor)
    {
        uint16 ponteiro = static_cast<uint16>(valor) << 8;

        for (uint i = 0; i < 256; i++)
        {
            this->oam.at(this->oam_endereco) = this->memoria->ler(ponteiro);
            this->oam_endereco++;
            ponteiro++;
        }

        this->dma->solicitar_dma(valor);
    }

    byte Ppu::get_dados()
    {
        byte valor = this->ler(this->v);
        if ((this->v%0x4000) < 0x3F00)
        {
            byte buffer_dados = this->buffer_dados;
            this->buffer_dados = valor;
            valor = buffer_dados;
        }
        else
        {
            this->buffer_dados = this->ler(this->v - 0x1000);
        }

        if (this->flag_incrementar == 0)
            this->v += 1;
        else
            this->v += 32;

        return valor;
    }

    void Ppu::set_dados(byte valor)
    {
        this->escrever(this->v, valor);
        
        if (this->flag_incrementar == false)
            this->v += 1;
        else
            this->v += 32;
    }

    uint16 Ppu::endereco_espelhado(byte modo, uint16 endereco)
    {
        uint16 endereco_espelhado = (endereco - 0x2000) % 0x1000;
        uint16 tabela = endereco_espelhado / 0x0400;
        uint16 offset = endereco_espelhado % 0x0400;
        return 0x2000 + espelhamento_tabela.at(modo).at(tabela) * 0x0400 + offset;
    }

    array<uint32, (256*240)>& Ppu::get_textura()
    {
        return *this->frente;
    }

    const array<uint32, (256*240)>& Ppu::get_textura() const
    {
        return *this->frente;
    }
}
