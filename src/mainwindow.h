#pragma once
#include <QMainWindow>
#include <QStringList>
class ImageView;
class QLabel;
class QDragEnterEvent;
class QDropEvent;

class QScrollArea;
class QWidget;
class QVBoxLayout;
class QToolBar;
class QTimer;
class QCloseEvent;
class QDragMoveEvent;
class MainWindow : public QMainWindow {
public:
    explicit MainWindow(QWidget *parent = nullptr);
    bool openImage(const QString &path);
protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
private:
    ImageView *view_;
    QLabel *info_;
    QToolBar *toolsBar_ = nullptr;
    QToolBar *thumbsBar_ = nullptr;
    QTimer *autoHideTimer_ = nullptr;
    QStringList files_;
    int index_ = -1;
    int thumbSize_ = 32;
    QString theme_ = "dark";
    QString currentPath_;
    QScrollArea *thumbsScroll_ = nullptr;
    QWidget *thumbsContainer_ = nullptr;
    QVBoxLayout *thumbsLayout_ = nullptr;
    void openDialog();
    void navigate(int step);
    void refreshFolder(const QString &path);
    void refreshThumbnails();
    void applyTheme(const QString &theme);
    void openSettings();
    void toggleFullscreen();
};
