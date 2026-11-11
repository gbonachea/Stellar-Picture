#include "imageview.h"
#include <QWheelEvent>
#include <QResizeEvent>
#include <QPixmap>
#include <QPainter>
#include <cmath>

ImageView::ImageView(QWidget *parent) : QGraphicsView(parent) {
    setScene(&scene_);
    item_ = scene_.addPixmap(QPixmap());
    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setBackgroundBrush(QColor(27, 30, 38));
    setFrameShape(QFrame::NoFrame);
}

bool ImageView::hasImage() const { return !item_->pixmap().isNull(); }

void ImageView::setImage(const QImage &image) {
    item_->setPixmap(QPixmap::fromImage(image));
    item_->setRotation(0);
    item_->setTransformOriginPoint(item_->boundingRect().center());
    scene_.setSceneRect(item_->sceneBoundingRect());
    fitImage();
}

void ImageView::scaleBy(double factor) {
    if (!hasImage()) return;
    // La escala real es la magnitud de la transformación (independiente de rotación)
    const double current = std::hypot(transform().m11(), transform().m12());
    if (current * factor < 0.02 || current * factor > 50.0) return;
    fitMode_ = false;
    scale(factor, factor);
}
void ImageView::zoomIn() { scaleBy(1.25); }
void ImageView::zoomOut() { scaleBy(0.8); }
void ImageView::actualSize() {
    if (!hasImage()) return;
    fitMode_ = false;
    resetTransform();
    centerOn(item_);
}
void ImageView::fitImage() {
    if (!hasImage()) return;
    fitMode_ = true;
    resetTransform();
    fitInView(item_, Qt::KeepAspectRatio);
}
void ImageView::rotateImage(int degrees) {
    if (!hasImage()) return;
    item_->setRotation(item_->rotation() + degrees);
    scene_.setSceneRect(item_->sceneBoundingRect());
    if (fitMode_) fitImage();
    else centerOn(item_);
}
void ImageView::wheelEvent(QWheelEvent *event) {
    if (!hasImage()) { event->ignore(); return; }
    if (event->angleDelta().y() > 0) zoomIn();
    else if (event->angleDelta().y() < 0) zoomOut();
    event->accept();
}
void ImageView::resizeEvent(QResizeEvent *event) {
    QGraphicsView::resizeEvent(event);
    if (fitMode_) fitImage();
}
