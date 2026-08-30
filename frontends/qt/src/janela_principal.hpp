#pragma once

#include <QElapsedTimer>
#include <QImage>
#include <QMainWindow>
#include <QTimer>

#include <array>
#include <filesystem>
#include <memory>

#include <nesbrasa/nesbrasa.hpp>

class QKeyEvent;
class QLabel;
class QPushButton;
class QStackedWidget;
class QEvent;

namespace nesbrasa::gui
{
    class Quadro final : public QWidget
    {
        const nucleo::Nes* nes;

    public:
        explicit Quadro(const nucleo::Nes* nes, QWidget* parent = nullptr);
        void atualizar();

    protected:
        void paintEvent(QPaintEvent* evento) override;
        QSize sizeHint() const override;
    };

    class JanelaPrincipal final : public QMainWindow
    {
    public:
        explicit JanelaPrincipal(QWidget* parent = nullptr);

    private:
        static constexpr int LARGURA_INICIAL = 600;
        static constexpr int ALTURA_INICIAL = 400;
        static constexpr int INTERVALO_TIMER_MS = 4;

        std::unique_ptr<nucleo::Nes> nes;
        Quadro* quadro = nullptr;
        QStackedWidget* paginas = nullptr;
        QPushButton* abrir_rom_vazio = nullptr;
        QTimer timer;
        QElapsedTimer relogio;
        std::array<int, 8> teclas{};
        std::filesystem::path caminho_configuracao;
        qint64 ultimo_tempo_frame = 0;
        double quadros_acumulados = 0.0;

        void abrir_rom();
        void abrir_configuracoes();
        void salvar_configuracoes();
        void carregar_configuracoes();
        void atualizar_controle(int tecla, bool pressionado);
        void atualizar_estado();
        void atualizar_emulacao();
        void mostrar_erro(const QString& mensagem);
        void mostrar_quadro();

        void keyPressEvent(QKeyEvent* evento) override;
        void keyReleaseEvent(QKeyEvent* evento) override;
        bool eventFilter(QObject* objeto, QEvent* evento) override;
    };

    int executar_aplicacao(int argc, char* argv[]);
}
