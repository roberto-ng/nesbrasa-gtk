#include "janela_principal.hpp"

#include "arquivo.hpp"

#include <QAction>
#include <QApplication>
#include <QCoreApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QEvent>
#include <QFileDialog>
#include <QFont>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QPaintEvent>
#include <QPushButton>
#include <QKeySequence>
#include <QStackedWidget>
#include <QToolBar>
#include <QVBoxLayout>

#include <algorithm>
#include <array>
#include <fstream>
#include <stdexcept>

namespace nesbrasa::gui
{
    using nucleo::Botao;
    using nucleo::Nes;

    namespace
    {
        constexpr std::array<int, 8> TECLAS_PADRAO = {
            Qt::Key_Z, Qt::Key_X, Qt::Key_Backspace, Qt::Key_Return,
            Qt::Key_Up, Qt::Key_Down, Qt::Key_Left, Qt::Key_Right,
        };
        constexpr std::array<const char*, 8> NOMES_BOTOES = {
            "A", "B", "Select", "Start", "Cima", "Baixo", "Esquerda", "Direita",
        };

        QString nome_tecla(int tecla)
        {
            const QString nome = QKeySequence(tecla).toString(QKeySequence::NativeText);
            return nome.isEmpty() ? QStringLiteral("Nenhuma") : nome;
        }
    }

    Quadro::Quadro(const nucleo::Nes* nes, QWidget* parent): QWidget(parent), nes(nes)
    {
        setFocusPolicy(Qt::StrongFocus);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        setAutoFillBackground(false);
    }

    void Quadro::atualizar()
    {
        update();
    }

    QSize Quadro::sizeHint() const
    {
        return {Nes::TELA_LARGURA * 2, Nes::TELA_ALTURA * 2};
    }

    void Quadro::paintEvent(QPaintEvent*)
    {
        QPainter painter(this);
        painter.fillRect(rect(), QColor(20, 20, 20));
        if (nes == nullptr || !nes->programa_carregado())
            return;

        const auto& textura = nes->get_textura();
        const QImage imagem(reinterpret_cast<const uchar*>(textura.data()),
            Nes::TELA_LARGURA, Nes::TELA_ALTURA,
            Nes::TELA_LARGURA * static_cast<int>(sizeof(nesbrasa::tipos::uint32)),
            QImage::Format_RGB32);
        const double escala = std::min(
            static_cast<double>(width()) / Nes::TELA_LARGURA,
            static_cast<double>(height()) / Nes::TELA_ALTURA);
        if (escala <= 0.0)
            return;
        const QSize tamanho(static_cast<int>(Nes::TELA_LARGURA * escala),
            static_cast<int>(Nes::TELA_ALTURA * escala));
        const QRect destino((width() - tamanho.width()) / 2,
            (height() - tamanho.height()) / 2, tamanho.width(), tamanho.height());
        painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
        painter.drawImage(destino, imagem);
    }

    JanelaPrincipal::JanelaPrincipal(QWidget* parent): QMainWindow(parent), nes(std::make_unique<Nes>())
    {
        teclas = TECLAS_PADRAO;
        caminho_configuracao = std::filesystem::path(QCoreApplication::applicationDirPath().toStdString()) /
            "nesbrasa-controls.conf";
        carregar_configuracoes();

        setWindowTitle(QStringLiteral("Nesbrasa"));
        setMinimumSize(512, 480);
        resize(LARGURA_INICIAL, ALTURA_INICIAL);

        auto* abrir = new QAction(QStringLiteral("Abrir ROM"), this);
        abrir->setShortcut(QKeySequence::Open);
        connect(abrir, &QAction::triggered, this, &JanelaPrincipal::abrir_rom);
        auto* configuracoes = new QAction(QStringLiteral("Configurações"), this);
        connect(configuracoes, &QAction::triggered, this, &JanelaPrincipal::abrir_configuracoes);
        auto* sair = new QAction(QStringLiteral("Sair"), this);
        sair->setShortcut(QKeySequence::Quit);
        connect(sair, &QAction::triggered, this, &QWidget::close);

        auto* arquivo = menuBar()->addMenu(QStringLiteral("Arquivo"));
        arquivo->addAction(abrir);
        arquivo->addAction(configuracoes);
        arquivo->addSeparator();
        arquivo->addAction(sair);

        auto* barra = addToolBar(QStringLiteral("Controles"));
        barra->setMovable(false);
        barra->addAction(abrir);
        barra->addAction(configuracoes);
#if defined(Q_OS_MACOS)
        // On macOS, QMainWindow toolbars otherwise become an in-window strip
        // beneath the title bar. Keep the actions in the native application
        // menu instead, matching normal macOS window behavior.
        menuBar()->setNativeMenuBar(true);
        setUnifiedTitleAndToolBarOnMac(true);
        barra->hide();
#endif

        paginas = new QStackedWidget(this);
        quadro = new Quadro(nes.get(), this);
        quadro->installEventFilter(this);
        paginas->addWidget(quadro);

        auto* vazio = new QWidget(this);
        auto* vazio_layout = new QVBoxLayout(vazio);
        vazio_layout->setContentsMargins(24, 24, 24, 24);
        vazio_layout->setSpacing(10);
        vazio_layout->addStretch();
        auto* texto = new QLabel(QStringLiteral("Nenhuma ROM carregada"), vazio);
        texto->setAlignment(Qt::AlignCenter);
        vazio_layout->addWidget(texto);
        abrir_rom_vazio = new QPushButton(QStringLiteral("Abrir ROM"), vazio);
        abrir_rom_vazio->setDefault(true);
        abrir_rom_vazio->setFixedWidth(210);
        connect(abrir_rom_vazio, &QPushButton::clicked, this, &JanelaPrincipal::abrir_rom);
        vazio_layout->addWidget(abrir_rom_vazio, 0, Qt::AlignHCenter);
        auto* config_vazio = new QPushButton(QStringLiteral("Configurações"), vazio);
        config_vazio->setFixedWidth(210);
        connect(config_vazio, &QPushButton::clicked, this, &JanelaPrincipal::abrir_configuracoes);
        vazio_layout->addWidget(config_vazio, 0, Qt::AlignHCenter);
        vazio_layout->addStretch();
        paginas->addWidget(vazio);
        setCentralWidget(paginas);

        connect(&timer, &QTimer::timeout, this, &JanelaPrincipal::atualizar_emulacao);
        timer.setTimerType(Qt::PreciseTimer);
        timer.setInterval(INTERVALO_TIMER_MS);
        timer.start();
        atualizar_estado();
        setFocusProxy(quadro);
        quadro->setFocus();
    }

    void JanelaPrincipal::abrir_rom()
    {
        const QString caminho = QFileDialog::getOpenFileName(
            this, QStringLiteral("Abrir ROM"), QString(),
            QStringLiteral("Arquivos NES (*.nes *.NES);;Todos os arquivos (*)"));
        if (caminho.isEmpty())
            return;

        try
        {
            nes->carregar_rom(ler_arquivo(caminho.toStdString()));
            ultimo_tempo_frame = 0;
            quadros_acumulados = 0.0;
            atualizar_estado();
            quadro->setFocus();
        }
        catch (const std::exception& erro)
        {
            mostrar_erro(QString::fromLocal8Bit(erro.what()));
        }
        catch (...)
        {
            mostrar_erro(QStringLiteral("Erro ao abrir arquivo"));
        }
    }

    void JanelaPrincipal::atualizar_estado()
    {
        paginas->setCurrentIndex(nes->programa_carregado() ? 0 : 1);
        if (!nes->programa_carregado())
        {
            ultimo_tempo_frame = 0;
            quadros_acumulados = 0.0;
        }
    }

    void JanelaPrincipal::atualizar_emulacao()
    {
        if (!nes->programa_carregado())
        {
            atualizar_estado();
            return;
        }

        constexpr double QUADROS_POR_SEGUNDO = 1789772.7272727273 / 29780.0;
        constexpr double MAX_TEMPO_DECORRIDO = 0.25;
        constexpr double MAX_QUADROS_ATRASADOS = 5.0;
        const qint64 agora = relogio.isValid() ? relogio.nsecsElapsed() / 1000 : 0;
        if (!relogio.isValid())
        {
            relogio.start();
            ultimo_tempo_frame = 0;
            return;
        }
        if (ultimo_tempo_frame == 0)
        {
            ultimo_tempo_frame = agora;
            return;
        }

        double decorrido = static_cast<double>(agora - ultimo_tempo_frame) / 1'000'000.0;
        ultimo_tempo_frame = agora;
        decorrido = std::clamp(decorrido, 0.0, MAX_TEMPO_DECORRIDO);
        quadros_acumulados = std::min(
            quadros_acumulados + decorrido * QUADROS_POR_SEGUNDO, MAX_QUADROS_ATRASADOS);

        bool atualizou = false;
        while (quadros_acumulados >= 1.0)
        {
            nes->avancar_quadro();
            quadros_acumulados -= 1.0;
            atualizou = true;
        }
        if (atualizou)
            mostrar_quadro();
    }

    void JanelaPrincipal::mostrar_quadro()
    {
        quadro->atualizar();
    }

    void JanelaPrincipal::abrir_configuracoes()
    {
        QDialog dialogo(this);
        dialogo.setWindowTitle(QStringLiteral("Configurações"));
        dialogo.setMinimumWidth(400);
        auto* layout = new QVBoxLayout(&dialogo);
        layout->setContentsMargins(28, 24, 28, 20);
        layout->setSpacing(16);

        auto* titulo = new QLabel(QStringLiteral("Controles do emulador"), &dialogo);
        auto fonte_titulo = titulo->font();
        fonte_titulo.setBold(true);
        fonte_titulo.setPointSize(fonte_titulo.pointSize() + 2);
        titulo->setFont(fonte_titulo);
        layout->addWidget(titulo);

        auto* ajuda = new QLabel(QStringLiteral("Selecione um controle e pressione a tecla desejada."));
        ajuda->setWordWrap(true);
        layout->addWidget(ajuda);
        auto* formulario = new QVBoxLayout;
        formulario->setSpacing(10);
        std::array<QPushButton*, 8> botoes{};
        auto atualizar_rotulos = [&]() {
            for (std::size_t i = 0; i < botoes.size(); ++i)
                botoes[i]->setText(nome_tecla(teclas[i]));
        };
        for (std::size_t i = 0; i < botoes.size(); ++i)
        {
            botoes[i] = new QPushButton(&dialogo);
            botoes[i]->setFocusPolicy(Qt::StrongFocus);
            botoes[i]->setFixedWidth(160);
            botoes[i]->installEventFilter(this);
            botoes[i]->setProperty("controle", static_cast<int>(i));
            auto* nome = new QLabel(QString::fromLatin1(NOMES_BOTOES[i]), &dialogo);
            nome->setFixedWidth(110);
            nome->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            auto* linha = new QHBoxLayout;
            linha->setSpacing(20);
            linha->addWidget(nome);
            linha->addWidget(botoes[i]);
            linha->setAlignment(Qt::AlignHCenter);
            formulario->addLayout(linha);
        }
        layout->addLayout(formulario);
        auto* restaurar = new QPushButton(QStringLiteral("Restaurar padrões"), &dialogo);
        restaurar->setFixedWidth(160);
        connect(restaurar, &QPushButton::clicked, this, [&]() {
            teclas = TECLAS_PADRAO;
            atualizar_rotulos();
        });
        auto* linha_restaurar = new QHBoxLayout;
        linha_restaurar->setSpacing(20);
        linha_restaurar->addSpacing(110);
        linha_restaurar->addWidget(restaurar);
        linha_restaurar->setAlignment(Qt::AlignHCenter);
        formulario->addLayout(linha_restaurar);
        auto* acoes = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialogo);
        connect(acoes, &QDialogButtonBox::accepted, &dialogo, &QDialog::accept);
        connect(acoes, &QDialogButtonBox::rejected, &dialogo, &QDialog::reject);
        layout->addWidget(acoes);
        atualizar_rotulos();
        const auto antes = teclas;
        if (dialogo.exec() == QDialog::Accepted)
            salvar_configuracoes();
        else
            teclas = antes;
    }

    void JanelaPrincipal::carregar_configuracoes()
    {
        std::ifstream arquivo(caminho_configuracao);
        for (int& tecla : teclas)
        {
            int valor = 0;
            if (!(arquivo >> valor))
                break;
            if (valor != 0)
                tecla = valor;
        }
    }

    void JanelaPrincipal::salvar_configuracoes()
    {
        std::ofstream arquivo(caminho_configuracao, std::ios::trunc);
        if (!arquivo)
            return;
        for (const int tecla : teclas)
            arquivo << tecla << '\n';
    }

    void JanelaPrincipal::atualizar_controle(int tecla, bool pressionado)
    {
        for (std::size_t i = 0; i < teclas.size(); ++i)
        {
            if (teclas[i] == tecla)
            {
                nes->set_botao(static_cast<Botao>(i), pressionado);
                return;
            }
        }
    }

    void JanelaPrincipal::keyPressEvent(QKeyEvent* evento)
    {
        if (!evento->isAutoRepeat())
            atualizar_controle(evento->key(), true);
        QMainWindow::keyPressEvent(evento);
    }

    void JanelaPrincipal::keyReleaseEvent(QKeyEvent* evento)
    {
        if (!evento->isAutoRepeat())
            atualizar_controle(evento->key(), false);
        QMainWindow::keyReleaseEvent(evento);
    }

    bool JanelaPrincipal::eventFilter(QObject* objeto, QEvent* evento)
    {
        if (evento->type() == QEvent::KeyPress || evento->type() == QEvent::KeyRelease)
        {
            auto* tecla = static_cast<QKeyEvent*>(evento);
            if (auto* botao = qobject_cast<QPushButton*>(objeto); botao != nullptr &&
                evento->type() == QEvent::KeyPress && !tecla->isAutoRepeat())
            {
                const int indice = botao->property("controle").toInt();
                if (indice >= 0 && indice < static_cast<int>(teclas.size()))
                {
                    teclas[static_cast<std::size_t>(indice)] = tecla->key();
                    botao->setText(nome_tecla(tecla->key()));
                    return true;
                }
            }
            if (objeto == quadro && !tecla->isAutoRepeat())
            {
                atualizar_controle(tecla->key(), evento->type() == QEvent::KeyPress);
                return true;
            }
        }
        return QMainWindow::eventFilter(objeto, evento);
    }

    void JanelaPrincipal::mostrar_erro(const QString& mensagem)
    {
        QMessageBox::critical(this, QStringLiteral("Nesbrasa"), mensagem);
    }

    int executar_aplicacao(int argc, char* argv[])
    {
        QApplication app(argc, argv);
        app.setApplicationName(QStringLiteral("Nesbrasa"));
        app.setApplicationDisplayName(QStringLiteral("Nesbrasa"));
        app.setOrganizationName(QStringLiteral("Nesbrasa"));
        JanelaPrincipal janela;
        janela.show();
        return app.exec();
    }
}
