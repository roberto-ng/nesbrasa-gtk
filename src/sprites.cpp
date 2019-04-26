#include <iostream>

#include "sprites.hpp"
#include "util.hpp"

using nesbrasa::nucleo::buscar_bit;

namespace nesbrasa::gui
{
    vector<guint8> criar_textura_sprites(const Nes& nes)
    {
        auto sprite = ler_sprite(nes, 0x03);

        vector<guint8> textura;
        for (auto valor : sprite)
        {
            switch (valor)
            {
                case 0:
                    textura.push_back(255);
                    textura.push_back(255);
                    textura.push_back(255);
                    break;
                
                case 1:
                    textura.push_back(14);
                    textura.push_back(14);
                    textura.push_back(14);
                    break;

                case 2:
                    textura.push_back(84);
                    textura.push_back(84);
                    textura.push_back(84);
                    break;

                case 3:
                    textura.push_back(0);
                    textura.push_back(0);
                    textura.push_back(0);
                    break;
                
                default:
                    throw string("Erro: Cor inválida");
                    break;
            }
        }

        return textura;
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

                sprite.push_back(valor);
            }
        }

        return sprite;
    }
}