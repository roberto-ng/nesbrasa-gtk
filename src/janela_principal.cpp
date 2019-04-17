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
    const string JanelaPrincipal::RECURSO_CAMINHO = "/nesbrasa/nesbrasa/emu/janela_principal.ui";

    JanelaPrincipal::JanelaPrincipal():
        Glib::ObjectBase("JanelaPrincipal")
    {
        this->builder = Gtk::Builder::create_from_resource(RECURSO_CAMINHO);
        this->builder->get_widget("raiz", this->raiz);
        this->builder->get_widget("label", this->label);

        this->add(*this->raiz);

        this->raiz->show();
        this->label->show();

        this->set_title("Nesbrasa");
    }
}
