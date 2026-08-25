/* janela_principal.cpp
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

#include "nesbrasa.gtk.hpp"

#include <limits.h>

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#elif defined(__linux__)
#include <unistd.h>
#elif defined(_WIN32)
#include <windows.h>
#endif
#include <fstream>

namespace nesbrasa::gui
{
    using std::make_unique;
    using std::runtime_error;
    using std::stringstream;
    using std::exception;
    using std::ifstream;
    using std::ofstream;
    using namespace std::string_literals;
    using nucleo::Botao;
    using namespace std::string_literals;

    static const array<guint, 8> TECLAS_PADRAO = {
        GDK_KEY_z, GDK_KEY_x, GDK_KEY_BackSpace, GDK_KEY_Return,
        GDK_KEY_Up, GDK_KEY_Down, GDK_KEY_Left, GDK_KEY_Right,
    };

    static const array<string, 8> NOMES_BOTOES = {
        "A", "B", "Select", "Start", "Cima", "Baixo", "Esquerda", "Direita",
    };

    static std::filesystem::path obter_diretorio_executavel()
    {
        try
        {
#if defined(__APPLE__)
            uint32_t tamanho = 0;
            _NSGetExecutablePath(nullptr, &tamanho);
            string caminho(tamanho, '\0');
            if (_NSGetExecutablePath(caminho.data(), &tamanho) == 0)
                return std::filesystem::path(caminho.c_str()).parent_path();
#elif defined(__linux__)
            array<char, PATH_MAX> caminho = {};
            const auto tamanho = readlink("/proc/self/exe", caminho.data(), caminho.size() - 1);
            if (tamanho > 0)
            {
                caminho[tamanho] = '\0';
                return std::filesystem::path(caminho.data()).parent_path();
            }
#elif defined(_WIN32)
            array<char, MAX_PATH> caminho = {};
            const auto tamanho = GetModuleFileNameA(nullptr, caminho.data(), caminho.size());
            if (tamanho > 0)
                return std::filesystem::path(caminho.data()).parent_path();
#endif
        }
        catch (...)
        {
        }

        return std::filesystem::current_path();
    }

    const guint JanelaPrincipal::ALTURA = 600;
    const guint JanelaPrincipal::LARGURA = 400;
    const string JanelaPrincipal::RECURSO_CAMINHO = "/nesbrasa/nesbrasa/emu/janela_principal.ui";

    static const double NES_TELA_LARGURA = Nes::TELA_LARGURA;
    static const double NES_TELA_ALTURA = Nes::TELA_ALTURA;

    JanelaPrincipal::JanelaPrincipal():
        Glib::ObjectBase("JanelaPrincipal")
    {
        this->nes = make_unique<Nes>();
        this->teclas = TECLAS_PADRAO;
        this->caminho_configuracao = obter_diretorio_executavel() / "nesbrasa-controls.conf";
        this->carregar_configuracoes();
        this->textura_tela = Gdk::Pixbuf::create(Gdk::Colorspace::COLORSPACE_RGB, false, 8, NES_TELA_LARGURA, NES_TELA_ALTURA);

        this->builder = Gtk::Builder::create_from_resource(RECURSO_CAMINHO);
        this->builder->get_widget("raiz", this->raiz);
        this->builder->get_widget("quadro", this->quadro);
        this->builder->get_widget("scroll", this->scroll);
        this->builder->get_widget("headerbar", this->headerbar);
        this->builder->get_widget("barra_menu", this->barra_menu);
        this->builder->get_widget("menu_item_sair", this->menu_item_sair);
        this->builder->get_widget("menu_item_configuracoes", this->menu_item_configuracoes);
        this->builder->get_widget("barra_mi_sair", this->barra_mi_sair);
        this->builder->get_widget("barra_mi_configuracoes", this->barra_mi_configuracoes);
        this->builder->get_widget("btn_abrir", this->btn_abrir);
        this->builder->get_widget("barra_mi_abrir", this->barra_mi_abrir);

        this->add(*this->raiz);

#if defined(_WIN32)
        // usar barra de menu no Windows
        this->barra_menu->show();
#else
        // usar headerbar 
        this->set_titlebar(*this->headerbar);
        this->barra_menu->hide();
#endif

        this->scroll->show_all_children();

        this->set_title("Nesbrasa");

        this->property_default_height() = LARGURA;
        this->property_height_request() = LARGURA;
        this->property_default_width() = ALTURA;
        this->property_width_request() = ALTURA;

        // conectar sinais de eventos

        this->menu_item_sair->signal_activate().connect(sigc::mem_fun(*this, &JanelaPrincipal::ao_fechar_janela));
        this->barra_mi_sair->signal_activate().connect(sigc::mem_fun(*this, &JanelaPrincipal::ao_fechar_janela));
        this->menu_item_configuracoes->signal_activate().connect(sigc::mem_fun(*this, &JanelaPrincipal::abrir_configuracoes));
        this->barra_mi_configuracoes->signal_activate().connect(sigc::mem_fun(*this, &JanelaPrincipal::abrir_configuracoes));

        this->btn_abrir->signal_clicked().connect(sigc::mem_fun(*this, &JanelaPrincipal::ao_clicar_btn_abrir));
        this->barra_mi_abrir->signal_activate().connect(sigc::mem_fun(*this, &JanelaPrincipal::ao_clicar_btn_abrir));

        this->quadro->signal_draw().connect(sigc::mem_fun(*this, &JanelaPrincipal::ao_desenhar_quadro));
        this->quadro->add_tick_callback(sigc::mem_fun(*this, &JanelaPrincipal::ao_atualizar));

        this->signal_key_press_event().connect(sigc::mem_fun(*this, &JanelaPrincipal::ao_pressionar_tecla), false);
        this->signal_key_release_event().connect(sigc::mem_fun(*this, &JanelaPrincipal::ao_soltar_tecla), false);
    }

    void JanelaPrincipal::ao_clicar_btn_abrir()
    {
        auto dialogo = Gtk::FileChooserNative::create(
            "Abrir", 
            *this, 
            Gtk::FileChooserAction::FILE_CHOOSER_ACTION_OPEN, 
            "_Open",
            "_Cancel"
        );

#if !defined(_WIN32)
        // Filtros de tipo de arquivo do GTK não funcionam no Windows

        // Mostrar apenas os arquivos do tipo NES
        auto filtro_nes = Gtk::FileFilter::create();
        filtro_nes->set_name("Arquivos NES");
        filtro_nes->add_mime_type("application/x-nes-rom");
        // macOS does not provide this MIME type, so also filter by extension.
        filtro_nes->add_pattern("*.nes");
        filtro_nes->add_pattern("*.NES");
        dialogo->add_filter(filtro_nes);

        // Mostrar todos os arquivos
        auto filtro_qualquer = Gtk::FileFilter::create();
        filtro_qualquer->set_name("Qualquer arquivo");
        filtro_qualquer->add_pattern("*");
        dialogo->add_filter(filtro_qualquer);
#endif

        try 
        {
            int resultado = dialogo->run(); 
            switch (resultado)
            {
                case Gtk::ResponseType::RESPONSE_ACCEPT:
                {
                    string caminho = dialogo->get_filename();                    
                    auto arquivo = ler_arquivo(caminho);
                    nes->carregar_rom(arquivo);

                    break;
                }

                case Gtk::ResponseType::RESPONSE_CANCEL:
                    break;

                default:
                    throw runtime_error("Erro ao abrir arquivo, resultado inválido"s);
                    break;
            }
        }
        catch (const exception& e)
        {
            Gtk::MessageDialog janela_dialogo(*this, e.what());
            janela_dialogo.run();
        }
        catch (...)
        {
            Gtk::MessageDialog janela_dialogo(*this, "Erro ao abrir arquivo");
            janela_dialogo.run();
        }

        // re-renderizar quadro
        this->quadro->queue_draw();
    }

    void JanelaPrincipal::ao_fechar_janela()
    {
        // fechar janela
        this->close();
    }

    void JanelaPrincipal::carregar_configuracoes()
    {
        try
        {
            ifstream arquivo(this->caminho_configuracao);
            for (guint i = 0; i < this->teclas.size(); ++i)
            {
                guint tecla = 0;
                if (!(arquivo >> tecla))
                    break;
                if (tecla != 0)
                    this->teclas[i] = tecla;
            }
        }
        catch (...)
        {
            this->teclas = TECLAS_PADRAO;
        }
    }

    void JanelaPrincipal::salvar_configuracoes()
    {
        try
        {
            ofstream arquivo(this->caminho_configuracao, std::ios::trunc);
            if (!arquivo)
                return;
            for (guint i = 0; i < this->teclas.size(); ++i)
            {
                arquivo << this->teclas[i] << '\n';
            }
        }
        catch (...)
        {
            // Config persistence is optional when running without installed schemas.
        }
    }

    void JanelaPrincipal::abrir_configuracoes()
    {
        Gtk::Dialog dialogo("Configurações", *this, true);
        dialogo.add_button("Cancelar", Gtk::ResponseType::RESPONSE_CANCEL);
        dialogo.add_button("Aplicar", Gtk::ResponseType::RESPONSE_OK);
        auto* acoes = dialogo.get_action_area();
        acoes->set_layout(Gtk::BUTTONBOX_EXPAND);
        acoes->set_homogeneous(true);
        acoes->set_spacing(10);
        dialogo.set_default_response(Gtk::ResponseType::RESPONSE_OK);
        dialogo.set_resizable(false);

        auto& conteudo = *dialogo.get_content_area();
        conteudo.set_border_width(20);
        conteudo.set_spacing(14);

        auto* titulo = Gtk::manage(new Gtk::Label());
        titulo->set_markup("<b>Controles do emulador</b>");
        titulo->set_halign(Gtk::ALIGN_START);
        conteudo.pack_start(*titulo, Gtk::PACK_SHRINK);

        auto* ajuda = Gtk::manage(new Gtk::Label(
            "Selecione um controle e pressione a tecla desejada."));
        ajuda->set_halign(Gtk::ALIGN_START);
        ajuda->set_line_wrap(true);
        conteudo.pack_start(*ajuda, Gtk::PACK_SHRINK);

        Gtk::Grid grade;
        grade.set_row_spacing(10);
        grade.set_column_spacing(24);
        grade.set_halign(Gtk::ALIGN_CENTER);
        conteudo.pack_start(grade, Gtk::PACK_SHRINK);

        array<Gtk::Button*, 8> botoes = {};
        auto atualizar_rotulos = [&]() {
            for (guint i = 0; i < botoes.size(); ++i)
            {
                const char* nome = gdk_keyval_name(this->teclas[i]);
                botoes[i]->set_label(nome != nullptr ? nome : "Nenhuma");
            }
        };

        for (guint i = 0; i < NOMES_BOTOES.size(); ++i)
        {
            auto* nome = Gtk::manage(new Gtk::Label(NOMES_BOTOES[i]));
            auto* botao = Gtk::manage(new Gtk::Button());
            botoes[i] = botao;
            nome->set_halign(Gtk::ALIGN_END);
            nome->set_size_request(140, -1);
            botao->set_size_request(180, 42);
            botao->set_hexpand(false);
            botao->set_can_focus(true);
            botao->add_events(Gdk::KEY_PRESS_MASK);
            botao->signal_key_press_event().connect([&, i](GdkEventKey* evento) {
                this->teclas[i] = evento->keyval;
                atualizar_rotulos();
                return true;
            }, false);
            grade.attach(*nome, 0, i, 1, 1);
            grade.attach(*botao, 1, i, 1, 1);
        }

        auto* restaurar = Gtk::manage(new Gtk::Button("Restaurar padrões"));
        restaurar->set_size_request(180, 38);
        restaurar->signal_clicked().connect([&, this]() {
            this->teclas = TECLAS_PADRAO;
            atualizar_rotulos();
        });
        grade.attach(*restaurar, 1, NOMES_BOTOES.size(), 1, 1);
        atualizar_rotulos();
        dialogo.show_all();

        if (dialogo.run() == Gtk::ResponseType::RESPONSE_OK)
        {
            this->salvar_configuracoes();
        }
        else
        {
            this->carregar_configuracoes();
        }
    }

    void JanelaPrincipal::atualizar_controle(guint tecla, bool pressionado)
    {
        for (guint i = 0; i < this->teclas.size(); ++i)
        {
            if (this->teclas[i] == tecla)
            {
                this->nes->set_botao(static_cast<Botao>(i), pressionado);
                return;
            }
        }
    }

    // Função chamada uma vez a cada frame do monitor
    bool JanelaPrincipal::ao_atualizar(const Glib::RefPtr<Gdk::FrameClock>& frame_clock)
    {
        if (!this->nes->programa_carregado())
        {
            return G_SOURCE_CONTINUE;
        }

        // dar foco ao quadro
        this->quadro->grab_focus();
        
        // Nes::avancar executa uma instrução completa e retorna quantos
        // ciclos de CPU ela consumiu. Avançar um número fixo de instruções
        // fazia a emulação correr várias vezes mais rápido que um quadro.
        this->nes->avancar_quadro();

        this->quadro->queue_draw();
        return G_SOURCE_CONTINUE;
    }

    // Função chamada quando é necessário renderizar o quadro.
    bool JanelaPrincipal::ao_desenhar_quadro(const Cairo::RefPtr<Cairo::Context>& cr)
    {
        if (!this->nes->programa_carregado())
        {
            return false;
        }

        auto pixels = this->textura_tela->get_pixels();
        const auto& textura = this->nes->get_textura();
        for (uint i = 0; i < textura.size(); i++) {
            uint32 valor = textura.at(i);
            pixels[i*3 + 0] = (valor & 0xFF0000) >> 4*4;
            pixels[i*3 + 1] = (valor & 0x00FF00) >> 4*2;
            pixels[i*3 + 2] = (valor & 0x0000FF);
        }

        const double largura = this->quadro->get_allocation().get_width();
        const double altura = this->quadro->get_allocation().get_height();

        double escala = 0;
        double largura_escalada = 0;
        double altura_escalada = 0;
        double pos_x = 0;
        double pos_y = 0;

        if (largura > altura)
        {
            // se a largura for maior ou igual que a altura
            escala = altura/NES_TELA_ALTURA;
            largura_escalada = NES_TELA_LARGURA*escala;
            altura_escalada = altura;
            
            // centralizar horizontalmente
            pos_x = (largura - largura_escalada) / 2.0;
        }
        else
        {
            // se a altura for maior que a largura 
            escala = largura/NES_TELA_LARGURA;
            largura_escalada = largura;
            altura_escalada = NES_TELA_ALTURA*escala;
            
            // centralizar verticalmente
            pos_y = (altura - altura_escalada) / 2.0;
        }

        auto textura_escalada = this->textura_tela->scale_simple(
            largura_escalada, 
            altura_escalada, 
            Gdk::InterpType::INTERP_NEAREST
        );

        // renderizar fundo
        auto estilo = this->quadro->get_style_context();
        estilo->render_background(cr, 0, 0, largura, altura);

        // renderizar o buffer da tela
        Gdk::Cairo::set_source_pixbuf(cr, textura_escalada, pos_x, pos_y);
        cr->rectangle(pos_x, pos_y, textura_escalada->get_width(), textura_escalada->get_height());
        cr->fill();

        return false;
    }

    bool JanelaPrincipal::ao_pressionar_tecla(GdkEventKey* evento)
    {
        this->atualizar_controle(evento->keyval, true);

        return false;
    }

    bool JanelaPrincipal::ao_soltar_tecla(GdkEventKey* evento)
    {
        this->atualizar_controle(evento->keyval, false);

        return false;
    }
}
