#pragma once
#include <iostream>
#include <array>
#include <memory>
#include <nesbrasa/cores.hpp>
#include <nesbrasa/tipos_numeros.hpp>
#include <nesbrasa/ports.hpp>

namespace nesbrasa::nucleo
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

