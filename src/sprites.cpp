#include <stdexcept>
#include <iostream>

#include "sprites.hpp"
#include "util.hpp"

namespace nesbrasa::gui
{
    using std::runtime_error;
    using nesbrasa::nucleo::buscar_bit;

    vector< vector<byte> > criar_textura_sprites(const Nes& nes)
    {
        const int chr_quantidade = nes.cartucho->get_chr_bancos_quantidade();
        const int chr_bancos_tam = nes.cartucho->CHR_BANCOS_TAMANHO;

        const int sprites_qtd = (chr_quantidade*chr_bancos_tam) / 16;

        vector< vector<byte> > sprites;
        for (int i = 0; i < sprites_qtd; i++)
        {
            auto sprite = ler_sprite(nes, i);
            sprites.push_back(sprite);
        }

        return sprites;
    }

    vector<byte> ler_sprite(const Nes& nes, uint pos)
    {        
        vector<byte> sprite;
        for (int i = 0; i < 8; i++)
        {
            byte byte1 = nes.cartucho->ler((pos*16) + i);
            byte byte2 = nes.cartucho->ler((pos*16) + i+8);

            for (int j = 7; j >= 0; j--)
            {
                byte bit1 = buscar_bit(byte1, j);
                byte bit2 = buscar_bit(byte2, j);
                byte valor = bit1 + bit2 + ((bit2 > 0)? 1 : 0);

                switch (valor)
                {
                    case 0:
                        sprite.push_back(0xFF);
                        sprite.push_back(0xFF);
                        sprite.push_back(0xFF);
                        break;
                    
                    case 1:
                        sprite.push_back(0xBC);
                        sprite.push_back(0xBC);
                        sprite.push_back(0xBC);
                        break;

                    case 2:
                        sprite.push_back(0x7C);
                        sprite.push_back(0x7C);
                        sprite.push_back(0x7C);
                        break;

                    case 3:
                        sprite.push_back(0x00);
                        sprite.push_back(0x00);
                        sprite.push_back(0x00);
                        break;
                    
                    default:
                        throw runtime_error("Erro: Cor inválida");
                        break;
                }
            }
        }

        return sprite;
    }
}