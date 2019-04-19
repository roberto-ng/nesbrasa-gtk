/* nesbrasa-gtk-window.cpp
 *
 * Copyright 2019 Roberto Nazareth
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "janela_principal.hpp"

namespace nesbrasa::gui
{
    const guint JanelaPrincipal::ALTURA = 600;
    const guint JanelaPrincipal::LARGURA = 400;
    const string JanelaPrincipal::RECURSO_CAMINHO = "/nesbrasa/nesbrasa/emu/janela_principal.ui";

    JanelaPrincipal::JanelaPrincipal():
        Glib::ObjectBase("JanelaPrincipal")
    {
        this->builder = Gtk::Builder::create_from_resource(RECURSO_CAMINHO);
        this->builder->get_widget("raiz", this->raiz);
        this->builder->get_widget("label", this->label);
        this->builder->get_widget("headerbar", this->headerbar);
        this->builder->get_widget("barra_menu", this->barra_menu);

        this->add(*this->raiz);

#if defined(_WIN32)
        // usar barra de menu no Windows
        this->barra_menu->show();
#else
        // usar headerbar no Gnu/Linux
        this->set_titlebar(*this->headerbar);
        this->barra_menu->hide();
#endif

        this->label->show();

        this->set_title("Nesbrasa");

        this->property_default_height() = LARGURA;
        this->property_height_request() = LARGURA;
        this->property_default_width() = ALTURA;
        this->property_width_request() = ALTURA;
    }
}
