/* main.cpp
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

#include <gtkmm/object.h>

#include "janela_principal.hpp"

using namespace nesbrasa::gui;

static void ao_ativar(Glib::RefPtr<Gtk::Application> app)
{
	auto janela = app->get_active_window();

	if (janela == nullptr)
	{
	    // criar janela
		janela = new JanelaPrincipal();
		janela->property_application() = app;
		janela->property_default_width() = 600;
		janela->property_default_height() = 300;

		app->add_window(*janela);
	}

	janela->present();
}

int main(int argc, char *argv[])
{
	Glib::RefPtr<Gtk::Application> app =
		Gtk::Application::create("nesbrasa.nesbrasa.emu", Gio::APPLICATION_FLAGS_NONE);

	app->signal_activate().connect(sigc::bind(&ao_ativar, app));

	int ret = app->run(argc, argv);

	return ret;
}
