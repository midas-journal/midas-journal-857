#include <stdio.h>
#include <stdlib.h>
#include <hyperflow.h>
#include "pipeline.h"
#if USE_QT
#include <QtCore/QSize>
#include <QtCore/QThread>
#include <QtGui/QApplication>
#include <QtGui/QHBoxLayout>
#include <QtGui/QKeyEvent>
#include "QEngineStatus.h"
#include "QImageViewer.h"
#endif

USING_NAMESPACE

#if USE_QT
class QPipelineThread: public QThread {
public:
  QPipelineThread(const char *filename, ExecutionEngine *ee, QImageViewer *iv, bool _itk, int _ncpu) {
    this->engine = ee;
    this->viewer = iv;
    this->imageFileName = filename;
    this->itk = _itk;
    this->ncpu = _ncpu;
  }
  void run();
private:
  ExecutionEngine* engine;
  QImageViewer*    viewer;
  const char*      imageFileName;
  bool itk;
  int ncpu;
};

void QPipelineThread::run() {
  runPipeline(this->imageFileName, this->engine, this->viewer, this->itk, this->ncpu);
}

class QEdgeDetectionWindow: public QWidget {
public:
  QEdgeDetectionWindow() {
    this->setWindowTitle("Panoramic Edge Detection");
  }
  
  virtual QSize sizeHint() const {
    return QSize(800, 600);
  }

  virtual void keyPressEvent(QKeyEvent *event) {
    if (event->key()==Qt::Key_Space)
      thread->start();
  }

  QPipelineThread *thread;
  
};
#endif

int main(int argc, char** argv) {
  if (argc<2) {
      fprintf(stderr, "Usage: ./edge_detection_itk [--itk] [--cpu <int>] filename\n");
      return 0;
  }
  int ncpu = 0, ngpu = 0;
  const char* filename = 0;
  bool useITK = false;
  for (int i=1; i<argc; i++) {
    if (strcmp(argv[i], "--cpu")==0) {
      ncpu = atoi(argv[++i]);
    }
    else if (strcmp(argv[i], "--itk")==0) {
      useITK = true;
    }
    else {
      filename = argv[i];
    }
  }
  
  ExecutionEngine *engine = new ExecutionEngine();
  if (const char *p = getenv("CPU"))
    if (ncpu==0)
      ncpu = strtoul(p, 0, 0);
  if (ncpu)
      engine->quickConfig(ncpu, 0);
  else
      engine->quickConfig();
  engine->start();
#if USE_QT
  QApplication *app = new QApplication(argc, argv);

  QEdgeDetectionWindow *mainWindow = new QEdgeDetectionWindow();
  mainWindow->setFocusPolicy(Qt::StrongFocus);
  QHBoxLayout *hbox = new QHBoxLayout();
  mainWindow->setLayout(hbox);  
  
  QImageViewer *viewer = new QImageViewer();
  hbox->addWidget(viewer);
  mainWindow->resize(800, 600);
  
  QEngineStatus *statusWidget = new QEngineStatus(engine);
  statusWidget->show();
  
  mainWindow->show();
  
  mainWindow->thread = new QPipelineThread(filename, engine, viewer, useITK, ncpu);
  app->exec();
#else
  runPipeline(filename, engine, NULL, useITK, ncpu);
#endif
  engine->stop();
  return 0;
}
