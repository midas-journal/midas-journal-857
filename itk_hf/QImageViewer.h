#ifndef QIMAGEVIEWER_H
#define QIMAGEVIEWER_H

#include <QtCore/QEvent>
#include <QtCore/QMutex>
#include <QtGui/QWidget>
#include <QtGui/QImage>
#include <QtGui/QLabel>

class QImageViewer: public QWidget {
  Q_OBJECT
public:
  QImageViewer(QWidget *parent=NULL);
  ~QImageViewer();

  virtual bool event(QEvent *e);

protected:
  virtual void closeEvent(QHideEvent *e);
  
public:
  QImage  image;
  QLabel* label;
  unsigned char* pCount;
};

#define PlaceImageEventType (QEvent::Type(QEvent::User+2))

class QPlaceImageEvent: public QEvent {
public:
  QPlaceImageEvent(QImage *img, int x, int y): QEvent(PlaceImageEventType) {
    this->image = img;
    this->placeX = x;
    this->placeY = y;
  }
                                     
  QImage *image;
  int placeX;
  int placeY;
};

#endif
