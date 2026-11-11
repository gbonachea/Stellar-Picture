#include "mainwindow.h"
#include "imageview.h"
#include <QToolBar>
#include <QAction>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QImageReader>
#include <QMessageBox>
#include <QStatusBar>
#include <QLabel>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QKeySequence>
#include <QShortcut>
#include <QFileInfoList>
#include <QSet>
#include <QToolButton>
#include <QStyle>
#include <QPainter>
#include <QFont>
#include <QVBoxLayout>
#include <QTimer>
#include <QSettings>
#include <QCloseEvent>
#include <QDragMoveEvent>
#include <QDialog>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QSpinBox>
#include <QLabel>
#include <QDialogButtonBox>
#include <QMenu>
#include <QTabWidget>
#include <QComboBox>
#include <QMessageBox>
#include <QPixmap>
#include <QScrollArea>
#include <QFrame>
#include <functional>
#include <vector>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("Stellar Picture");
    QSettings settings("StellarPicture", "StellarPicture");
    if (settings.contains("geometry"))
        restoreGeometry(settings.value("geometry").toByteArray());
    else
        resize(960, 650);
    theme_ = settings.value("theme", "dark").toString();
    setAcceptDrops(true);
    view_ = new ImageView(this);
    setCentralWidget(view_);
    view_->setAcceptDrops(true);
    view_->viewport()->setAcceptDrops(true);
    setAcceptDrops(true);
    view_->setMouseTracking(true);
    view_->viewport()->setMouseTracking(true);
    view_->viewport()->installEventFilter(this);
    view_->installEventFilter(this);
    setMouseTracking(true);
    installEventFilter(this);

    auto makeIcon = [](const char *themeName, const QString &symbol) {
        QIcon themeIcon = QIcon::fromTheme(themeName);
        if (!themeIcon.isNull()) {
            QPixmap pm = themeIcon.pixmap(24, 24);
            QPixmap colored(pm.size());
            colored.fill(Qt::transparent);
            QPainter p(&colored);
            p.drawPixmap(0, 0, pm);
            p.setCompositionMode(QPainter::CompositionMode_SourceIn);
            p.fillRect(colored.rect(), QColor("#7aa2ff"));
            p.end();
            return QIcon(colored);
        }
        QPixmap pm(28, 28);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QColor("#7aa2ff"));
        QFont f = p.font();
        f.setPointSize(14);
        f.setBold(true);
        p.setFont(f);
        p.drawText(pm.rect(), Qt::AlignCenter, symbol);
        p.end();
        return QIcon(pm);
    };

    toolsBar_ = new QToolBar("Herramientas", this);
    toolsBar_->setMovable(false);
    toolsBar_->setFloatable(false);
    toolsBar_->setAllowedAreas(Qt::BottomToolBarArea);
    addToolBar(Qt::BottomToolBarArea, toolsBar_);
    // Abrir: solo icono (recoloreado al color del tema)
    {
        QIcon stdIcon = style()->standardIcon(QStyle::SP_DialogOpenButton);
        QPixmap pm = stdIcon.pixmap(24, 24);
        QPixmap colored(pm.size());
        colored.fill(Qt::transparent);
        QPainter p(&colored);
        p.drawPixmap(0, 0, pm);
        p.setCompositionMode(QPainter::CompositionMode_SourceIn);
        p.fillRect(colored.rect(), QColor("#7aa2ff"));
        p.end();
        QAction *openAction = toolsBar_->addAction(colored, QString());
        openAction->setToolTip("Abrir imagen (Ctrl+O)");
        openAction->setShortcut(QKeySequence::Open);
        connect(openAction, &QAction::triggered, this, [this]{ openDialog(); });
    }
    toolsBar_->addSeparator();
    {
        QAction *prev = toolsBar_->addAction(makeIcon("go-previous", "◀"), QString());
        prev->setToolTip("Anterior (←)");
        prev->setShortcut(Qt::Key_Left);
        connect(prev, &QAction::triggered, this, [this]{ navigate(-1); });
    }
    {
        QAction *next = toolsBar_->addAction(makeIcon("go-next", "▶"), QString());
        next->setToolTip("Siguiente (→)");
        next->setShortcut(Qt::Key_Right);
        connect(next, &QAction::triggered, this, [this]{ navigate(1); });
    }
    toolsBar_->addSeparator();

    // Acciones de ajuste directamente en la barra
    {
        struct Entry { const char *theme; QString symbol; QString tip; QKeySequence shortcut; std::function<void()> cb; };
        const std::vector<Entry> entries = {
            {"zoom-in", "＋", "Acercar", QKeySequence(Qt::Key_Plus), [this]{ view_->zoomIn(); }},
            {"zoom-out", "−", "Alejar", QKeySequence(Qt::Key_Minus), [this]{ view_->zoomOut(); }},
            {"zoom-original", "1:1", "Tamaño real", QKeySequence(Qt::Key_1), [this]{ view_->actualSize(); }},
            {"zoom-fit-best", "Fit", "Ajustar", QKeySequence(Qt::Key_F), [this]{ view_->fitImage(); }},
            {"object-rotate-left", "↶", "Girar izquierda", QKeySequence(Qt::Key_BracketLeft), [this]{ view_->rotateImage(-90); }},
            {"object-rotate-right", "↷", "Girar derecha", QKeySequence(Qt::Key_BracketRight), [this]{ view_->rotateImage(90); }},
        };
        for (const auto &e : entries) {
            QAction *a = toolsBar_->addAction(makeIcon(e.theme, e.symbol), QString());
            a->setToolTip(e.tip);
            a->setShortcut(e.shortcut);
            connect(a, &QAction::triggered, this, [cb = e.cb]{ cb(); });
        }
    }
    toolsBar_->addSeparator();
    {
        QAction *fs = toolsBar_->addAction(makeIcon("view-fullscreen", " (F11)"), QString());
        fs->setToolTip("Pantalla completa (F11)");
        fs->setShortcut(Qt::Key_F11);
        connect(fs, &QAction::triggered, this, [this]{ toggleFullscreen(); });
    }
    auto *escape = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(escape, &QShortcut::activated, this, [this]{ if (isFullScreen()) showNormal(); });
    // Botón de configuración (ventana con pestañas)
    {
        auto *settingsBtn = new QToolButton(toolsBar_);
        {
            QPixmap pm(":/icons/settings.png");
            if (!pm.isNull()) {
                QPixmap colored(pm.size());
                colored.fill(Qt::transparent);
                QPainter p(&colored);
                p.drawPixmap(0, 0, pm);
                p.setCompositionMode(QPainter::CompositionMode_SourceIn);
                p.fillRect(colored.rect(), QColor("#7aa2ff"));
                p.end();
                settingsBtn->setIcon(QIcon(colored));
            } else {
                settingsBtn->setIcon(makeIcon("preferences-system", "⚙"));
            }
        }
        settingsBtn->setToolTip("Configuración");
        settingsBtn->setAutoRaise(true);
        settingsBtn->setIconSize(QSize(20, 20));
        settingsBtn->setStyleSheet("QToolButton{border:0;border-radius:8px;padding:6px;} QToolButton:hover{background:#414b60;}");
        connect(settingsBtn, &QToolButton::clicked, this, [this]{ openSettings(); });
        toolsBar_->addWidget(settingsBtn);
    }

    info_ = new QLabel("", this);
    auto *spacer = new QWidget(toolsBar_);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolsBar_->addWidget(spacer);
    toolsBar_->addWidget(info_);
    statusBar()->hide();

    // Barra vertical de miniaturas en el lado derecho
    thumbsBar_ = new QToolBar("Miniaturas", this);
    thumbsBar_->setMovable(false);
    thumbsBar_->setFloatable(false);
    thumbsBar_->setOrientation(Qt::Vertical);
    thumbsBar_->setAllowedAreas(Qt::RightToolBarArea);
    addToolBar(Qt::RightToolBarArea, thumbsBar_);

    thumbsScroll_ = new QScrollArea(thumbsBar_);
    thumbsScroll_->setFixedWidth(84);
    thumbsScroll_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    thumbsScroll_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    thumbsScroll_->setWidgetResizable(true);
    thumbsScroll_->setFrameShape(QFrame::NoFrame);
    thumbsScroll_->setStyleSheet("QScrollArea{background:transparent;border:0;} QScrollArea > QWidget > QWidget{background:transparent;} QScrollBar:vertical{background:transparent;width:6px;margin:2px;} QScrollBar::handle:vertical{background:#7aa2ff;border-radius:3px;min-height:20px;} QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical{height:0;} QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical{background:transparent;}");
    thumbsScroll_->viewport()->setAutoFillBackground(false);
    thumbsContainer_ = new QWidget(thumbsScroll_);
    thumbsContainer_->setStyleSheet("background:transparent;");
    thumbsLayout_ = new QVBoxLayout(thumbsContainer_);
    thumbsLayout_->setContentsMargins(4, 6, 4, 6);
    thumbsLayout_->setSpacing(6);
    thumbsScroll_->setWidget(thumbsContainer_);
    thumbsScroll_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    thumbsBar_->addWidget(thumbsScroll_);
    thumbsBar_->setFixedWidth(92);

    autoHideTimer_ = new QTimer(this);
    autoHideTimer_->setInterval(40);
    connect(autoHideTimer_, &QTimer::timeout, this, [this]{
        if (!isFullScreen()) return;
        QPoint p = mapFromGlobal(QCursor::pos());
        const int w = width();
        const int h = height();
        if (toolsBar_) toolsBar_->setVisible(p.y() >= h - 70);
        if (thumbsBar_) thumbsBar_->setVisible(p.x() >= w - 95);
    });

    applyTheme(theme_);
}

void MainWindow::openDialog() {
    QString path = QFileDialog::getOpenFileName(this, "Abrir imagen", QString(),
        "Imágenes (*.png *.jpg *.jpeg *.bmp *.gif *.webp *.tif *.tiff *.ico *.svg);;Todos los archivos (*)");
    if (!path.isEmpty()) openImage(path);
}

void MainWindow::refreshFolder(const QString &path) {
    QDir dir(QFileInfo(path).absolutePath());
    QSet<QString> supported;
    for (const QByteArray &format : QImageReader::supportedImageFormats())
        supported.insert(QString::fromLatin1(format).toLower());
    // Formatos habituales por si el plugin Qt no los reporta
    supported << "jpg" << "jpeg" << "tif" << "tiff" << "png" << "bmp" << "gif" << "webp";

    files_.clear();
    const QFileInfoList entries = dir.entryInfoList(QDir::Files | QDir::Readable, QDir::Name | QDir::IgnoreCase);
    for (const QFileInfo &file : entries) {
        if (supported.contains(file.suffix().toLower()))
            files_ << file.absoluteFilePath();
    }

    const QString target = QFileInfo(path).absoluteFilePath();
    index_ = files_.indexOf(target);
    if (index_ < 0) {
        const QString name = QFileInfo(path).fileName().toLower();
        for (int i = 0; i < files_.size(); ++i) {
            if (QFileInfo(files_.at(i)).fileName().toLower() == name) {
                index_ = i;
                break;
            }
        }
    }
}

void MainWindow::refreshThumbnails() {
    if (!thumbsLayout_ || !thumbsContainer_) return;
    // Limpiar
    QLayoutItem *child;
    while ((child = thumbsLayout_->takeAt(0)) != nullptr) {
        if (child->widget()) child->widget()->deleteLater();
        delete child;
    }

    for (int i = 0; i < files_.size(); ++i) {
        const QString &f = files_.at(i);
        QPixmap pm(f);
        if (pm.isNull()) continue;
        pm = pm.scaled(thumbSize_, thumbSize_, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        auto *btn = new QToolButton(thumbsContainer_);
        btn->setIcon(QIcon(pm));
        btn->setIconSize(QSize(thumbSize_, thumbSize_));
        btn->setToolTip(QFileInfo(f).fileName());
        btn->setAutoRaise(true);
        btn->setCursor(Qt::PointingHandCursor);
        QString style = "QToolButton{border:2px solid transparent;border-radius:6px;padding:2px;}";
        if (i == index_) style += "QToolButton{border:2px solid #7aa2ff;}";
        btn->setStyleSheet(style);
        connect(btn, &QToolButton::clicked, this, [this, f]{ openImage(f); });
        thumbsLayout_->addWidget(btn);
    }
    thumbsLayout_->addStretch();
    thumbsContainer_->adjustSize();
}

bool MainWindow::openImage(const QString &path) {
    QImageReader reader(path);
    reader.setAutoTransform(true);
    const QImage image = reader.read();
    if (image.isNull()) {
        QMessageBox::warning(this, "No se pudo abrir", reader.errorString());
        return false;
    }
    view_->setImage(image);
    currentPath_ = QFileInfo(path).absoluteFilePath();
    refreshFolder(currentPath_);
    refreshThumbnails();
    setWindowTitle(QFileInfo(path).fileName() + " — Stellar Picture");
    info_->setText(QString("%1 × %2  |  %3 de %4").arg(image.width()).arg(image.height())
        .arg(index_ >= 0 ? index_ + 1 : 1).arg(files_.size()));
    return true;
}

void MainWindow::navigate(int step) {
    if (files_.isEmpty() || index_ < 0) return;
    const int count = files_.size();
    const int next = (index_ + step + count) % count;
    openImage(files_.at(next));
}
void MainWindow::openSettings() {
    QDialog dlg(this);
    dlg.setWindowTitle("Configuración");
    dlg.resize(420, 320);
    QString dlgStyle;
    if (theme_ == "light") {
        dlgStyle = "QDialog{background:#fffff8;color:#3a3a3a;} QWidget{color:#3a3a3a;} QTabWidget::pane{border:1px solid #e0dcc8;border-radius:8px;} QTabBar::tab{background:#faf8f0;color:#3a3a3a;padding:6px 12px;border-radius:6px;} QTabBar::tab:selected{background:#fffff8;} QCheckBox{color:#3a3a3a;} QSpinBox{background:#faf8f0;color:#3a3a3a;border:1px solid #e0dcc8;border-radius:4px;padding:2px;} QComboBox{background:#faf8f0;color:#3a3a3a;border:1px solid #e0dcc8;border-radius:4px;padding:2px;} QComboBox QAbstractItemView{background:#faf8f0;color:#3a3a3a;} QLabel{color:#3a3a3a;} QPushButton{background:#faf8f0;color:#3a3a3a;border:1px solid #e0dcc8;border-radius:6px;padding:6px 12px;} QPushButton:hover{background:#f2efe4;}";
    } else {
        dlgStyle = "QDialog{background:#1b1e26;color:#e9edf5;} QWidget{color:#e9edf5;} QTabWidget::pane{border:1px solid #3f475a;border-radius:8px;} QTabBar::tab{background:#252a35;color:#dce3ef;padding:6px 12px;border-radius:6px;} QTabBar::tab:selected{background:#2a303d;} QCheckBox{color:#e9edf5;} QSpinBox{background:#252a35;color:#e9edf5;border:1px solid #3f475a;border-radius:4px;padding:2px;} QComboBox{background:#252a35;color:#e9edf5;border:1px solid #3f475a;border-radius:4px;padding:2px;} QComboBox QAbstractItemView{background:#252a35;color:#e9edf5;} QLabel{color:#e9edf5;} QPushButton{background:#2a303d;color:#e9edf5;border:1px solid #3f475a;border-radius:6px;padding:6px 12px;} QPushButton:hover{background:#414b60;}";
    }
    dlg.setStyleSheet(dlgStyle);

    auto *tabs = new QTabWidget(&dlg);
    // Pestaña Ajustes
    auto *settingsPage = new QWidget(&dlg);
    auto *settingsLay = new QVBoxLayout(settingsPage);
    auto *showThumbs = new QCheckBox("Mostrar miniaturas", settingsPage);
    showThumbs->setChecked(thumbsBar_ && thumbsBar_->isVisible());
    settingsLay->addWidget(showThumbs);
    auto *sizeLabel = new QLabel("Tamaño de miniaturas:", settingsPage);
    settingsLay->addWidget(sizeLabel);
    auto *sizeSpin = new QSpinBox(settingsPage);
    sizeSpin->setRange(24, 96);
    sizeSpin->setValue(thumbSize_);
    settingsLay->addWidget(sizeSpin);

    auto *themeLabel = new QLabel("Apariencia:", settingsPage);
    settingsLay->addWidget(themeLabel);
    auto *themeCombo = new QComboBox(settingsPage);
    themeCombo->addItem("Oscuro", "dark");
    themeCombo->addItem("Claro", "light");
    int idx = themeCombo->findData(theme_);
    if (idx >= 0) themeCombo->setCurrentIndex(idx);
    settingsLay->addWidget(themeCombo);
    settingsLay->addStretch();
    tabs->addTab(settingsPage, "Ajustes");

    // Pestaña Atajos
    auto *shortcutsPage = new QWidget(&dlg);
    auto *shortcutsLay = new QVBoxLayout(shortcutsPage);
    auto *shortcutsLabel = new QLabel(
        "Atajos de teclado:\n\n"
        "Ctrl+O — Abrir imagen\n"
        "← / → — Imagen anterior / siguiente\n"
        "+ / − — Acercar / Alejar\n"
        "F — Ajustar a ventana\n"
        "1 — Tamaño real\n"
        "[ / ] — Girar izquierda / derecha\n"
        "F11 — Pantalla completa\n"
        "Esc — Salir de pantalla completa", shortcutsPage);
    shortcutsLabel->setWordWrap(true);
    shortcutsLay->addWidget(shortcutsLabel);
    shortcutsLay->addStretch();
    tabs->addTab(shortcutsPage, "Atajos");

    // Pestaña Acerca de
    auto *aboutPage = new QWidget(&dlg);
    auto *aboutLay = new QVBoxLayout(aboutPage);
    auto *aboutLabel = new QLabel(
        "Stellar Picture\n"
        "Visor de imágenes ligero para Linux.\n\n"
        "Escrito en C++ y Qt6 Widgets.\n"
        "Versión 1.0", aboutPage);
    aboutLabel->setWordWrap(true);
    aboutLay->addWidget(aboutLabel);
    aboutLay->addStretch();
    tabs->addTab(aboutPage, "Acerca de");

    auto *lay = new QVBoxLayout(&dlg);
    lay->addWidget(tabs);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    lay->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() == QDialog::Accepted) {
        if (thumbsBar_) thumbsBar_->setVisible(showThumbs->isChecked());
        thumbSize_ = sizeSpin->value();
        QString newTheme = themeCombo->currentData().toString();
        QSettings settings("StellarPicture", "StellarPicture");
        settings.setValue("theme", newTheme);
        applyTheme(newTheme);
        refreshThumbnails();
    }
}

void MainWindow::applyTheme(const QString &theme) {
    theme_ = theme;
    QString dark = "QMainWindow{background:#1b1e26;color:#e9edf5;} QToolBar{background:#2a303d;spacing:6px;padding:8px;margin:8px;border:1px solid #3f475a;border-radius:14px;} QToolBar::separator{background:transparent;border:0;} QToolButton{color:#edf1f8;padding:6px 10px;border-radius:8px;border:0;} QToolButton:hover{background:#414b60;} QToolButton:pressed{background:#343c4f;} QMenu{background:#2a303d;color:#edf1f8;border:1px solid #3f475a;border-radius:12px;padding:6px;} QMenu::item{padding:6px 18px;border-radius:6px;} QMenu::item:selected{background:#414b60;} QStatusBar{background:#252a35;color:#dce3ef;} QLabel{color:#dce3ef;}";
    QString light = "QMainWindow{background:#fffff8;color:#3a3a3a;} QToolBar{background:#faf8f0;spacing:6px;padding:8px;margin:8px;border:1px solid #e0dcc8;border-radius:14px;} QToolBar::separator{background:transparent;border:0;} QToolButton{color:#3a3a3a;padding:6px 10px;border-radius:8px;border:0;} QToolButton:hover{background:#f2efe4;} QToolButton:pressed{background:#e8e4d8;} QMenu{background:#faf8f0;color:#3a3a3a;border:1px solid #e0dcc8;border-radius:12px;padding:6px;} QMenu::item{padding:6px 18px;border-radius:6px;} QMenu::item:selected{background:#f2efe4;} QStatusBar{background:#faf8f0;color:#3a3a3a;} QLabel{color:#3a3a3a;}";
    setStyleSheet(theme == "light" ? light : dark);
    if (view_) view_->setBackgroundBrush(theme == "light" ? QColor(255, 255, 248) : QColor(27, 30, 38));
}

void MainWindow::toggleFullscreen() {
    if (isFullScreen()) {
        showNormal();
        if (toolsBar_) toolsBar_->show();
        if (thumbsBar_) thumbsBar_->show();
        if (autoHideTimer_) autoHideTimer_->stop();
    } else {
        showFullScreen();
        if (toolsBar_) toolsBar_->hide();
        if (thumbsBar_) thumbsBar_->hide();
        if (autoHideTimer_) autoHideTimer_->start();
    }
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event) {
    // Drag & drop (soporte incluso sobre la vista)
    if (watched == view_ || (view_ && watched == view_->viewport())) {
        if (event->type() == QEvent::DragEnter) {
            auto *de = static_cast<QDragEnterEvent*>(event);
            if (de->mimeData()->hasUrls()) { de->acceptProposedAction(); return true; }
        } else if (event->type() == QEvent::DragMove) {
            auto *de = static_cast<QDragMoveEvent*>(event);
            if (de->mimeData()->hasUrls()) { de->acceptProposedAction(); return true; }
        } else if (event->type() == QEvent::Drop) {
            auto *de = static_cast<QDropEvent*>(event);
            for (const QUrl &url : de->mimeData()->urls()) {
                if (url.isLocalFile() && openImage(url.toLocalFile())) {
                    de->acceptProposedAction();
                    return true;
                }
            }
            return true;
        }
    }
    if (isFullScreen() && event->type() == QEvent::MouseMove) {
        QPoint p = mapFromGlobal(QCursor::pos());
        const int w = width();
        const int h = height();
        if (toolsBar_) toolsBar_->setVisible(p.y() >= h - 70);
        if (thumbsBar_) thumbsBar_->setVisible(p.x() >= w - 95);
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::closeEvent(QCloseEvent *event) {
    QSettings settings("StellarPicture", "StellarPicture");
    settings.setValue("geometry", saveGeometry());
    QMainWindow::closeEvent(event);
}
void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls()) event->acceptProposedAction();
}
void MainWindow::dragMoveEvent(QDragMoveEvent *event) {
    if (event->mimeData()->hasUrls()) event->acceptProposedAction();
}
void MainWindow::dropEvent(QDropEvent *event) {
    bool opened = false;
    for (const QUrl &url : event->mimeData()->urls()) {
        if (url.isLocalFile() && openImage(url.toLocalFile())) {
            opened = true;
            break;
        }
    }
    if (opened) event->acceptProposedAction();
    else event->ignore();
}
