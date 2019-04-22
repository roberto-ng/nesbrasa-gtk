/* nesbrasa-gtk-window.h
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

#pragma once

#include <string>
#include <memory>
#include <gtkmm.h>

#include "nesbrasa.hpp"

using std::string;
using std::shared_ptr;
using nesbrasa::nucleo::Nes;

namespace nesbrasa::gui
{
    class JanelaPrincipal : public Gtk::Window
    {
    private:
        static const guint ALTURA;
        static const guint LARGURA;
        static const string RECURSO_CAMINHO;
        
        shared_ptr<Nes> nes;

        Glib::RefPtr<Gtk::Builder> builder;
        
        Gtk::HeaderBar* headerbar;
        Gtk::MenuBar*   barra_menu;

        Gtk::Button* btn_abrir;
        Gtk::Box *   raiz;
        Gtk::Label*  label;

        Gtk::MenuItem* menu_item_sair;
        Gtk::MenuItem* barra_mi_sair;
        Gtk::MenuItem* barra_mi_abrir;

    public:
        JanelaPrincipal();

        // sinais de eventos

        void ao_clicar_btn_abrir();
        void ao_fechar_janela();
    };
}
