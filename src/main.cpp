#include <QApplication>
#include <QFileInfo>
#include <QIcon>
#include "mainwindow.h"
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Stellar Picture");
    app.setDesktopFileName("stellar-picture");
    app.setWindowIcon(QIcon(":/icons/stellarpicture.png"));
    MainWindow window;
    window.show();
    if (argc > 1) window.openImage(QString::fromLocal8Bit(argv[1]));
    return app.exec();
}
