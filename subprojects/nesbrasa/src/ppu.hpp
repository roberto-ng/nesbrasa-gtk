/* ppu.hpp
 *
 * Copyright 2019 Roberto Nazareth <nazarethroberto97@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <cstdint>
#include <array>
#include <memory>

#include "memoria.hpp"

// referencias utilizadas:
// https://wiki.nesdev.com/w/index.php/PPU_registers
// http://nemulator.com/files/nes_emu.txt

namespace nesbrasa::nucleo
{
    using std::array;
    using std::shared_ptr;
    
    extern array< array<uint16, 4>, 5> espelhamento_tabela;

    class Ppu
    {
    private:
        Memoria* memoria;

        int ciclo;
        int scanline;
        uint64 frame;

        array<byte, 0x20>  paletas;
        array<byte, 0x800> tabelas_de_nomes;
        array<byte, 0x100> oam;
        // texturas RGB representando a tela do NES
        array<uint32, (256*240)> frente;
        array<uint32, (256*240)> fundo;

        // registradores internos
        uint16 v;
        uint16 t;
        byte   x;
        bool   w;
        bool   f;

        byte ultimo_valor; // último valor escrito na ppu

        // flags do nmi
        bool nmi_ocorreu;
        bool nmi_output;
        bool nmi_anterior;
        byte nmi_atrasar;

        // membros relacionados às texturas de fundo
        byte tabela_de_nomes_byte;
        byte tabela_de_atributos_byte;
        byte tile_byte_maior;
        byte tile_byte_menor;
        uint64 tile_dados;

        int sprites_qtd;
        array<uint64, 8> sprites_padroes;
        array<byte, 8>   sprites_posicoes;
        array<byte, 8>   sprites_prioridades;
        array<int, 8>   sprites_indices;        
        
        uint16 vram_incrementar;
        
        // PPUCTRL - $2000
        byte flag_nametable_base;
        bool flag_incrementar;
        bool flag_padrao_sprite;
        bool flag_padrao_fundo;
        bool flag_sprite_altura;
        bool flag_mestre_escravo;
        uint16 sprite_padrao_tabela_endereco;

        // PPUMASK - $2001
        bool flag_enfase_b;
        bool flag_enfase_g;
        bool flag_enfase_r;
        bool flag_sprite_habilitar;
        bool flag_fundo_habilitar;
        bool flag_sprite_habilitar_col_esquerda;
        bool flag_fundo_habilitar_col_esquerda;
        bool flag_escala_cinza;

        // PPUSTATUS - $2002
        bool flag_sprite_zero;
        bool flag_sprite_transbordamento;

        // OAMADDR - $2003
        byte oam_endereco;

        // PPUDATA - $2007
        byte buffer_dados;

    public:        
        Ppu(Memoria* memoria);

        void reiniciar();
        void atualizar();
        void avancar();

        byte ler(Nes *nes, uint16 endereco);
        void escrever(Nes *nes, uint16 endereco, byte valor);

        byte registrador_ler(uint16 endereco);
        void registrador_escrever(Nes *nes, uint16 endereco, byte valor);

        byte ler_paleta(uint16 endereco);
        void escrever_paleta(uint16 endereco, byte valor);

        array<uint32, (256*240)>& get_textura();

    private:
        byte buscar_pixel_fundo();
        byte buscar_pixel_sprite(byte& indice);
        byte buscar_cor_fundo(byte dados);
        byte buscar_cor_pixel(byte dados);
        uint32 buscar_padrao_sprite(int i, int linha);
        void renderizar_pixel();

        void executar_ciclo_vblank();
        void encerrar_ciclo_vblank();
        void alterar_nmi();

        void buscar_byte_tabela_de_nomes();
        void buscar_byte_tabela_de_atributos();
        void buscar_tile_byte_menor();
        void buscar_tile_byte_maior();
        void tile_guardar_dados();
        void avaliar_sprites();

        void copiar_x();
        void copiar_y();
        void mudar_scroll_x();
        void mudar_scroll_y();

        void set_controle(byte valor);
        void set_mascara(byte  valor);
        byte get_estado();
        void set_oam_enderco(byte valor);
        void set_oam_dados(byte valor);
        byte get_oam_dados();
        void set_scroll(byte valor);
        void set_endereco(byte valor);
        void set_omd_dma(Nes *nes, byte valor);
        byte get_dados();
        void set_dados(Nes *nes, byte valor);

        uint16 endereco_espelhado(byte modo, uint16 endereco);

        void set_textura_valor(array<byte, (256*240)>& textura, int x, int y, int valor);
    };
}