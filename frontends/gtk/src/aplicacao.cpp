#include "nesbrasa.gtk.hpp"

#if defined(_WIN32)
#include <windows.h>
#endif

namespace nesbrasa::gui
{
    static const string APP_ID = "nesbrasa.nesbrasa.emu";

    static void ao_ativar(Glib::RefPtr<Gtk::Application> app)
    {
        auto janela = app->get_active_window();
        if (janela == nullptr)
        {
            janela = new JanelaPrincipal();
            janela->property_application() = app;
            app->add_window(*janela);
        }

#if defined(_WIN32)
        SetEnvironmentVariable("GTK_CSD", "0");
        auto tela = Gdk::Screen::get_default();
        if (tela.get() != nullptr)
        {
            auto configuracao = Gtk::Settings::get_for_screen(tela);
            configuracao->property_gtk_theme_name() = "win32";
        }
#endif
        janela->present();
    }

    int executar_aplicacao(int argc, char* argv[])
    {
        auto app = Gtk::Application::create(APP_ID, Gio::APPLICATION_FLAGS_NONE);
        app->signal_activate().connect(sigc::bind(&ao_ativar, app));
        return app->run(argc, argv);
    }
}
