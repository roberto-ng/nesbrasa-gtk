module;

#include <array>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <gtkmm.h>

export module nesbrasa.gtk;
export import nesbrasa.core;

export namespace nesbrasa::gui
{
    using std::array;
    using std::optional;
    using std::string;
    using std::unique_ptr;
        using nesbrasa::nucleo::Nes;
    using nesbrasa::tipos::uint32;

    class JanelaPrincipal : public Gtk::Window
    {
    private:
        static const guint ALTURA;
        static const guint LARGURA;
        static const string RECURSO_CAMINHO;
        Glib::RefPtr<Gdk::Pixbuf> textura_tela;
        unique_ptr<Nes> nes;
        Glib::RefPtr<Gtk::Builder> builder;
        Gtk::HeaderBar* headerbar;
        Gtk::MenuBar* barra_menu;
        Gtk::MenuItem* menu_item_sair;
        Gtk::MenuItem* menu_item_configuracoes;
        Gtk::MenuItem* barra_mi_sair;
        Gtk::MenuItem* barra_mi_configuracoes;
        Gtk::MenuItem* barra_mi_abrir;
        Gtk::Button* btn_abrir;
        Gtk::Box* raiz;
        Gtk::DrawingArea* quadro;
        Gtk::ScrolledWindow* scroll;
        array<guint, 8> teclas;
        std::filesystem::path caminho_configuracao;
        void abrir_configuracoes();
        void atualizar_controle(guint tecla, bool pressionado);
        void carregar_configuracoes();
        void salvar_configuracoes();
    public:
        JanelaPrincipal();
        void ao_clicar_btn_abrir();
        void ao_fechar_janela();
        bool ao_atualizar(const Glib::RefPtr<Gdk::FrameClock>& frame_clock);
        bool ao_desenhar_quadro(const Cairo::RefPtr<Cairo::Context>& cr);
        bool ao_pressionar_tecla(GdkEventKey* evento);
        bool ao_soltar_tecla(GdkEventKey* evento);
    };

    std::vector<nesbrasa::tipos::byte> ler_arquivo(string caminho);
    int executar_aplicacao(int argc, char* argv[]);
}
