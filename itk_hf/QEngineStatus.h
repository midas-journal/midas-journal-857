#ifndef QENGINESTATUS_H
#define QENGINESTATUS_H

#include <QtCore/QEvent>
#include <QtGui/QLabel>
#include <QtGui/QWidget>
#include <hyperflow.h>

USING_NAMESPACE

class QEngineStatus: public QWidget {
  Q_OBJECT
public:
  QEngineStatus(ExecutionEngine *ee, QWidget *parent=NULL);
  
  QSize sizeHint() const { return QSize(400, 50); }

  static void onBeforeExecution(void *arg);
  static void onAfterExecution(void *arg);

private:
  ExecutionEngine*        engine;
};

class QVPEStatus: public QLabel {
  Q_OBJECT
public:
  QVPEStatus(VirtualProcessingElement *e);
  virtual ~QVPEStatus() {}
  VirtualProcessingElement *vpe;

  virtual bool event(QEvent *e);
};

#endif
