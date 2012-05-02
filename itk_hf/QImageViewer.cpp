#include "QImageViewer.h"
#include <QtCore/QCoreApplication>
#include <QtCore/QMutex>
#include <QtGui/QImage>
#include <QtGui/QPixmap>
#include <QtGui/QHBoxLayout>
#include <QtGui/QResizeEvent>

#include <inttypes.h>
#include <stdio.h>

QImageViewer::QImageViewer(QWidget *parent)
    : QWidget(parent) {
  for (int y=0; y<this->image.height(); y++) {
    if (y<this->image.height()/2)
      memset(this->image.scanLine(y), 0, this->image.bytesPerLine());
    else
      memset(this->image.scanLine(y), 0xFF, this->image.bytesPerLine());
  }
  QHBoxLayout *hbox = new QHBoxLayout();
  this->setLayout(hbox);
  this->label = new QLabel();
  this->label->setPixmap(QPixmap::fromImage(this->image));
  this->label->setScaledContents(true);
  hbox->addWidget(this->label);
  this->pCount = (unsigned char*)malloc(this->image.width()*this->image.height());
  memset(this->pCount, 0, this->image.width()*this->image.height());
}

QImageViewer::~QImageViewer() {
  free(this->pCount);
  this->pCount = NULL;
}

bool QImageViewer::event(QEvent *e) { 
  if (e->type()==PlaceImageEventType) {
    QPlaceImageEvent *pie = (QPlaceImageEvent*)e;
    for (int sy=0; sy<pie->image->height(); sy++) {
      if (pie->placeY+sy >= this->image.height())
        break;
      unsigned *src = (unsigned*)pie->image->scanLine(sy);
      unsigned *dst = (unsigned*)this->image.scanLine(pie->placeY+sy)+pie->placeX;
      for (int i=0; i<pie->image->width(); i++) {
        if (i+pie->placeX >= this->image.width())
          break;
        uint8_t *udst = (uint8_t*)(dst+i);
        uint8_t *usrc = (uint8_t*)(src+i);
        for (int j=0; j<3; j++) {
          udst[j] = usrc[j];
        }
      }
    }
    this->label->setPixmap(QPixmap::fromImage(this->image));
    this->label->update();
    return true;
  }
  else if (e->type()==QEvent::Resize) {
    QRect r = this->label->frameRect();
    this->image = QImage(r.width(), r.height(), QImage::Format_RGB32);
    for (int y=0; y<this->image.height(); y++) {
      if (y<this->image.height()/2)
        memset(this->image.scanLine(y), 0, this->image.bytesPerLine());
      else
        memset(this->image.scanLine(y), 0xFF, this->image.bytesPerLine());
    }
    this->label->setPixmap(QPixmap::fromImage(this->image));
    this->label->setMinimumSize(QSize(1,1));
    this->label->update();
    this->layout()->invalidate();
    return true;
  }
  return QWidget::event(e);
}

void QImageViewer::closeEvent(QHideEvent *e) {
  exit(0);
}
