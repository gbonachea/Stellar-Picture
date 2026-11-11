#pragma once
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QImage>

class ImageView : public QGraphicsView {
public:
    explicit ImageView(QWidget *parent = nullptr);
    void setImage(const QImage &image);
    void zoomIn();
    void zoomOut();
    void actualSize();
    void fitImage();
    void rotateImage(int degrees);
    bool hasImage() const;
protected:
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
private:
    QGraphicsScene scene_;
    QGraphicsPixmapItem *item_ = nullptr;
    bool fitMode_ = true;
    void scaleBy(double factor);
};
