#include "sprites.hpp"
#include "util.hpp"

using nesbrasa::nucleo::buscar_bit;

namespace nesbrasa::gui
{
    vector< vector<guint8> > criar_textura_sprites(Nes& nes)
    {
        int sprites_qtd = (nes.cartucho.get_chr_quantidade()*0x2000) / 16;
        //sprites_qtd = 4;

        vector< vector<guint8> > sprites;
        for (int i = 0; i < sprites_qtd; i++)
        {
            auto sprite = ler_sprite(nes, i);
            sprites.push_back(sprite);
        }

        return sprites;
    }

    vector<guint8> ler_sprite(const Nes& nes, guint pos)
    {
        auto& chr_rom = nes.cartucho.chr;
        
        vector<guint8> sprite;
        for (int i = 0; i < 8; i++)
        {
            guint8 byte1 = chr_rom.at((pos*16) + i);
            guint8 byte2 = chr_rom.at((pos*16) + i+8);

            for (int j = 7; j >= 0; j--)
            {
                guint8 bit1 = buscar_bit(byte1, j);
                guint8 bit2 = buscar_bit(byte2, j);
                guint8 valor = bit1 + bit2 + ((bit2 > 0)? 1 : 0);

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
                        throw string("Erro: Cor inválida");
                        break;
                }
            }
        }

        return sprite;
    }
}