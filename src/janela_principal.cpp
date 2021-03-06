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

#include <iostream>
#include <sstream>
#include <gtkmm.h>
#include <glib.h>

#include "arquivo.hpp"
#include "janela_principal.hpp"

namespace nesbrasa::gui
{
    using std::make_unique;
    using std::runtime_error;
    using std::stringstream;
    using std::exception;
    using nucleo::Botao;
    using namespace std::string_literals;

    const guint JanelaPrincipal::ALTURA = 600;
    const guint JanelaPrincipal::LARGURA = 400;
    const string JanelaPrincipal::RECURSO_CAMINHO = "/nesbrasa/nesbrasa/emu/janela_principal.ui";

    static const double NES_TELA_LARGURA = 256;
    static const double NES_TELA_ALTURA = 240;

    JanelaPrincipal::JanelaPrincipal():
        Glib::ObjectBase("JanelaPrincipal")
    {
        this->nes = make_unique<Nes>();
        this->textura_tela = Gdk::Pixbuf::create(Gdk::Colorspace::COLORSPACE_RGB, false, 8, NES_TELA_LARGURA, NES_TELA_ALTURA);

        this->builder = Gtk::Builder::create_from_resource(RECURSO_CAMINHO);
        this->builder->get_widget("raiz", this->raiz);
        this->builder->get_widget("quadro", this->quadro);
        this->builder->get_widget("scroll", this->scroll);
        this->builder->get_widget("headerbar", this->headerbar);
        this->builder->get_widget("barra_menu", this->barra_menu);
        this->builder->get_widget("menu_item_sair", this->menu_item_sair);
        this->builder->get_widget("barra_mi_sair", this->barra_mi_sair);
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

    // Função chamada uma vez a cada frame do monitor
    bool JanelaPrincipal::ao_atualizar(const Glib::RefPtr<Gdk::FrameClock>& frame_clock)
    {
        if (!this->nes->is_programa_carregado)
        {
            return G_SOURCE_CONTINUE;
        }

        // dar foco ao quadro
        this->quadro->grab_focus();
        
        const int ciclos = 29780; 
        for (int i = 0; i < ciclos; i++)
        {
            this->nes->avancar();
        }

        this->quadro->queue_draw();
        return G_SOURCE_CONTINUE;
    }

    // Função chamada quando é necessário renderizar o quadro.
    bool JanelaPrincipal::ao_desenhar_quadro(const Cairo::RefPtr<Cairo::Context>& cr)
    {
        if (!this->nes->is_programa_carregado)
        {
            return false;
        }

        auto pixels = this->textura_tela->get_pixels();
        auto textura = this->nes->ppu.get_textura();
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
        switch (evento->keyval)
        {
            case GDK_KEY_z:
                this->nes->controle_1.set_valor(Botao::A, true);
                break;

            case GDK_KEY_x:
                this->nes->controle_1.set_valor(Botao::B, true);
                break;

            case GDK_KEY_BackSpace:
                this->nes->controle_1.set_valor(Botao::SELECT, true);
                break;

            case GDK_KEY_Return:
                this->nes->controle_1.set_valor(Botao::START, true);
                break;

            case GDK_KEY_Up:
                this->nes->controle_1.set_valor(Botao::CIMA, true);
                break;
            
            case GDK_KEY_Down:
                this->nes->controle_1.set_valor(Botao::BAIXO, true);
                break;
            
            case GDK_KEY_Left:
                this->nes->controle_1.set_valor(Botao::ESQUERDA, true);
                break;

            case GDK_KEY_Right:
                this->nes->controle_1.set_valor(Botao::DIREITA, true);
                break;
            
            default: break;
        }

        return false;
    }

    bool JanelaPrincipal::ao_soltar_tecla(GdkEventKey* evento)
    {
        switch (evento->keyval)
        {
            case GDK_KEY_z:
                this->nes->controle_1.set_valor(Botao::A, false);
                break;

            case GDK_KEY_x:
                this->nes->controle_1.set_valor(Botao::B, false);
                break;

            case GDK_KEY_BackSpace:
                this->nes->controle_1.set_valor(Botao::SELECT, false);
                break;

            case GDK_KEY_Return:
                this->nes->controle_1.set_valor(Botao::START, false);
                break;

            case GDK_KEY_Up:
                this->nes->controle_1.set_valor(Botao::CIMA, false);
                break;
            
            case GDK_KEY_Down:
                this->nes->controle_1.set_valor(Botao::BAIXO, false);
                break;
            
            case GDK_KEY_Left:
                this->nes->controle_1.set_valor(Botao::ESQUERDA, false);
                break;

            case GDK_KEY_Right:
                this->nes->controle_1.set_valor(Botao::DIREITA, false);
                break;
            
            default: break;
        }

        return false;
    }
}
