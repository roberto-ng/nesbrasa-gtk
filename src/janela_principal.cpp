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

#include <iostream>
#include <gtkmm.h>

#include "janela_principal.hpp"
#include "arquivo.hpp"
#include "sprites.hpp"

using std::make_shared;
using std::runtime_error;
using std::exception;

namespace nesbrasa::gui
{
    using namespace std::string_literals;

    const guint JanelaPrincipal::ALTURA = 600;
    const guint JanelaPrincipal::LARGURA = 400;
    const string JanelaPrincipal::RECURSO_CAMINHO = "/nesbrasa/nesbrasa/emu/janela_principal.ui";

    JanelaPrincipal::JanelaPrincipal():
        Glib::ObjectBase("JanelaPrincipal")
    {
        this->nes = make_shared<Nes>();

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
    }

    void JanelaPrincipal::ao_clicar_btn_abrir()
    {
        auto dialogo = gtk_file_chooser_native_new("Abrir arquivo", nullptr, 
                            GTK_FILE_CHOOSER_ACTION_OPEN, "_Open","_Cancel");

#if !defined(_WIN32)
        // Filtros de tipo de arquivo não funcionam com o escolhedor de arquivo
        // padrão do windows, então os usaremos apenas em outras plataformas

        // Mostrar apenas os arquivos do tipo NES
        auto filtro_nes = gtk_file_filter_new();
        gtk_file_filter_set_name(filtro_nes, "Arquivos NES");
        gtk_file_filter_add_mime_type(filtro_nes, "application/x-nes-rom");
        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialogo), filtro_nes);

        // Mostrar todos os arquivos
        auto filtro_qualquer = gtk_file_filter_new();
        gtk_file_filter_set_name(filtro_qualquer, "Qualquer arquivo");
        gtk_file_filter_add_pattern(filtro_qualquer, "*");
        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialogo), filtro_qualquer);
#endif

        try 
        {
            int resultado = gtk_native_dialog_run(GTK_NATIVE_DIALOG(dialogo));    
            switch (resultado)
            {
                case GTK_RESPONSE_ACCEPT:
                {
                    auto c_arquivo_nome = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialogo));
                    string arquivo_nome(c_arquivo_nome);
                    g_free(c_arquivo_nome);
                    
                    auto arquivo = ler_arquivo(arquivo_nome);
                    nes->carregar_rom(arquivo);
                    break;
                }

                case GTK_RESPONSE_CANCEL:
                    break;

                default:
                    throw runtime_error("Erro ao abrir arquivo, resultado inválido"s);
                    break;
            }
        }
        catch (const exception& e)
        {
            Gtk::MessageDialog janela_dialogo(*this, e.what(), Gtk::MessageType::MESSAGE_ERROR);
            janela_dialogo.run();
        }
        catch (...)
        {
            Gtk::MessageDialog janela_dialogo(*this, "Erro ao abrir arquivo", Gtk::MessageType::MESSAGE_ERROR);
            janela_dialogo.run();
        }

        g_object_unref(dialogo);

        // re-renderizar quadro
        this->quadro->queue_draw();
    }

    void JanelaPrincipal::ao_fechar_janela()
    {
        // fechar janela
        this->close();
    }

    // Sinal ativado quando é necessário renderizar o quadro.
    // Renderiza uma textura na tela
    bool JanelaPrincipal::ao_desenhar_quadro(const Cairo::RefPtr<Cairo::Context>& cr)
    {
        const int largura = this->quadro->get_allocation().get_width();
        const int altura = this->quadro->get_allocation().get_height();

        if (!this->nes->cartucho.possui_rom_carregada())
        {
            // deixar o quadro completamente preto
            cr->set_source_rgb(0, 0, 0);
            cr->rectangle(0, 0, largura, altura);
            cr->fill();

            return true;
        }

        // deixar o quadro completamente preto
        cr->set_source_rgb(0, 0, 0);
        cr->rectangle(0, 0, largura, altura);
        cr->fill();

        auto texturas = criar_textura_sprites(*this->nes);

        int borda = 2;
        int display_largura = 0x20*8*5 + 0x20*borda;
        int display_altura = (texturas.size()/0x20)*8*5 + 0x20*borda;
        auto display = Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, false, 8, display_largura, display_altura);
        // preenche o display com um fundo branco
        display->fill(0x00000000);

        int y = 0;
        for (guint i = 0; i < texturas.size(); i++)
        {
            auto& textura = texturas.at(i);
            // transforma a textura em um pixbuf para que ela possa ser entendida pelo GTK
            auto pixbuf = Gdk::Pixbuf::create_from_data(textura.data(), Gdk::COLORSPACE_RGB, 
                            false, 8, 8, 8, 3*8);

            // aumenta o tamanho do pixbuf para ele preencher o quadro 
            auto pixbuf_escalado = pixbuf->scale_simple(pixbuf->get_width()*5, pixbuf->get_height()*5, 
                                        Gdk::InterpType::INTERP_NEAREST);

            int x = (((i+1)%0x20)-1) * (pixbuf_escalado->get_width() + borda);
            if (i > 0 && ((i+1)%0x20) == 0)
            {
                y += pixbuf_escalado->get_height() + borda;
            }
            
            if ((x >= 0 && y >= 0) &&
                (x + pixbuf_escalado->get_width())  < display->get_width() && 
                (y + pixbuf_escalado->get_height()) < display->get_height())
            {
                pixbuf_escalado->copy_area(0, 0, 
                                    pixbuf_escalado->get_width(), 
                                    pixbuf_escalado->get_height(), 
                                    display, x, y);
            }
        }

        Gdk::Cairo::set_source_pixbuf(cr, display, 0, 0);
        cr->rectangle(0, 0, display->get_width(), display->get_height());
        cr->fill();

        // faz a barra de scroll obedecer o tamanho do quadro
        this->quadro->set_size_request(display->get_width(), display->get_height());
        
        // retornar true para parar de re-renderizar
        return true;
    }
}
