#pragma once

#include <algorithm>
#include <array>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <gtkmm.h>

#include <nesbrasa/nesbrasa.hpp>
namespace nesbrasa::gui
{
    using std::array;
    using std::optional;
    using std::string;
    using std::unique_ptr;
    using nesbrasa::nucleo::Nes;

    class JanelaPrincipal : public Gtk::Window
    {
    private:
        static const guint ALTURA;
        static const guint LARGURA;
        static const string RECURSO_CAMINHO;
        std::array<Cairo::RefPtr<Cairo::ImageSurface>, 2> superficies_textura;
        std::array<const void*, 2> dados_textura = {};
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
        Gtk::Button* btn_abrir_rom;
        Gtk::Button* btn_configuracoes;
        Gtk::Box* raiz;
        Gtk::DrawingArea* quadro;
        Gtk::ScrolledWindow* scroll;
        array<guint, 8> teclas;
        std::filesystem::path caminho_configuracao;
        gint64 ultimo_tempo_frame = 0;
        double quadros_acumulados = 0.0;
        void abrir_configuracoes();
        void atualizar_controle(guint tecla, bool pressionado);
        void carregar_configuracoes();
        void salvar_configuracoes();
    public:
        JanelaPrincipal();
        void ao_clicar_btn_abrir();
        void ao_abrir_configuracoes();
        void ao_fechar_janela();
        bool ao_atualizar(const Glib::RefPtr<Gdk::FrameClock>& frame_clock);
        bool ao_desenhar_quadro(const Cairo::RefPtr<Cairo::Context>& cr);
        bool ao_pressionar_tecla(GdkEventKey* evento);
        bool ao_soltar_tecla(GdkEventKey* evento);
    };

    std::vector<nesbrasa::tipos::byte> ler_arquivo(string caminho);
    int executar_aplicacao(int argc, char* argv[]);
}
