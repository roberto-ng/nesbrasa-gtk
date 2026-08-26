#include "nesbrasa.gtk.hpp"

#if defined(_WIN32)
#include <windows.h>
#endif

namespace nesbrasa::gui
{
    static const string APP_ID = "nesbrasa.nesbrasa.emu";

#if defined(__APPLE__)
    static JanelaPrincipal* janela_principal_ativa(const Glib::RefPtr<Gtk::Application>& app)
    {
        return dynamic_cast<JanelaPrincipal*>(app->get_active_window());
    }

    static void configurar_menu_macos(const Glib::RefPtr<Gtk::Application>& app)
    {
        auto abrir = Gio::SimpleAction::create("abrir");
        abrir->signal_activate().connect([app](const Glib::VariantBase&) {
            if (auto* janela = janela_principal_ativa(app))
                janela->ao_clicar_btn_abrir();
        });
        app->add_action(abrir);

        auto configuracoes = Gio::SimpleAction::create("configuracoes");
        configuracoes->signal_activate().connect([app](const Glib::VariantBase&) {
            if (auto* janela = janela_principal_ativa(app))
                janela->ao_abrir_configuracoes();
        });
        app->add_action(configuracoes);

        auto sair = Gio::SimpleAction::create("sair");
        sair->signal_activate().connect([app](const Glib::VariantBase&) {
            if (auto* janela = janela_principal_ativa(app))
                janela->ao_fechar_janela();
        });
        app->add_action(sair);

        auto arquivo = Gio::Menu::create();
        arquivo->append("Abrir", "app.abrir");
        arquivo->append("Configurações", "app.configuracoes");
        arquivo->append("Sair", "app.sair");

        auto menu = Gio::Menu::create();
        menu->append_submenu("Arquivo", arquivo);
        app->set_menubar(menu);

        app->set_accels_for_action("app.abrir", {"<Primary>O"});
        app->set_accels_for_action("app.sair", {"<Primary>Q"});
    }
#endif

    static void ao_ativar(Glib::RefPtr<Gtk::Application> app)
    {
        auto janela = app->get_active_window();
        JanelaPrincipal* nova_janela = nullptr;
        if (janela == nullptr)
        {
            nova_janela = new JanelaPrincipal();
            nova_janela->property_application() = app;
            app->add_window(*nova_janela);

#if defined(__APPLE__)
            if (!app->lookup_action("abrir"))
                configurar_menu_macos(app);
#endif
            janela = nova_janela;
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
#if defined(__APPLE__)
        // Must be set before GTK initializes so windows use the native title bar.
        g_setenv("GTK_CSD", "0", TRUE);
#endif
        auto app = Gtk::Application::create(APP_ID, Gio::APPLICATION_FLAGS_NONE);
        app->signal_activate().connect(sigc::bind(&ao_ativar, app));
        return app->run(argc, argv);
    }
}
