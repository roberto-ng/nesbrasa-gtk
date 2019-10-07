/* janela_principal.hpp
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
#include <array>
#include <memory>
#include <optional>
#include <gtkmm.h>

#include "nesbrasa.hpp"

namespace nesbrasa::gui
{
    using std::string;
    using std::unique_ptr;
    using std::optional;
    using std::array;
    using nesbrasa::nucleo::Nes;

    class JanelaPrincipal : public Gtk::Window
    {
    private:
        static const guint ALTURA;
        static const guint LARGURA;
        static const string RECURSO_CAMINHO;
        
        array<guint8, 256*240*3> textura;
        Glib::RefPtr<Gdk::Pixbuf> pixbuf;
        unique_ptr<Nes> nes;
        optional<double> ultimo_tempo;

        Glib::RefPtr<Gtk::Builder> builder;
        
        Gtk::HeaderBar* headerbar;
        Gtk::MenuBar*   barra_menu;

        Gtk::MenuItem* menu_item_sair;
        Gtk::MenuItem* barra_mi_sair;
        Gtk::MenuItem* barra_mi_abrir;
        Gtk::Button*   btn_abrir;

        Gtk::Box*            raiz;
        Gtk::DrawingArea*    quadro;
        Gtk::ScrolledWindow* scroll;

    public:
        JanelaPrincipal();

        // sinais de eventos

        void ao_clicar_btn_abrir();
        void ao_fechar_janela();
        bool ao_atualizar(const Glib::RefPtr<Gdk::FrameClock>& frame_clock);
        bool ao_desenhar_quadro(const Cairo::RefPtr<Cairo::Context>& cr);
        bool ao_pressionar_tecla(GdkEventKey* evento);
        bool ao_soltar_tecla(GdkEventKey* evento);
    };
}
