#include "sprites.hpp"

namespace nesbrasa::gui
{
    vector<guint8> criar_textura_sprites(const Nes& nes)
    {
        vector<guint8> teste {
            1, 0, 0, 0,
            0, 0, 1, 1,
            1, 0, 0, 0,
            0, 1, 0, 1,
        };

        vector<guint8> textura;
        for (auto valor : teste)
        {
            switch (valor)
            {
                case 0:
                    textura.push_back(0);
                    textura.push_back(0);
                    textura.push_back(0);
                    break;
                
                case 1:
                    textura.push_back(255);
                    textura.push_back(255);
                    textura.push_back(255);
                    break;
                
                default:
                    break;
            }
        }

        return textura;
    }
}