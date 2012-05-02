#include "QEngineStatus.h"
#include <QtCore/QCoreApplication>
#include <QtGui/QGridLayout>
#include <QtGui/QLabel>
#include <QtGui/QFont>

QEngineStatus::QEngineStatus(ExecutionEngine *ee, QWidget *parent)
    : QWidget(parent) {
  this->setWindowTitle("HyperFlow Execution Engine Status");
  this->engine = ee;

  QGridLayout *grid = new QGridLayout();
  this->setLayout(grid);

  QFont labelFont, statusFont;
  labelFont.setPointSize(12);
  statusFont.setPointSize(12);
    
  grid->addWidget(new QLabel("<center><b><font color=\"black\">Virtual Processing Element</font></b></center>"), 0, 0);
  grid->addWidget(new QLabel("<center><b><font color=\"black\">Executing</font></b></center>"), 0, 1);
  for (unsigned i=0; i<this->engine->getNumberOfVPEs(); i++) {
    VirtualProcessingElement *vpe = this->engine->getVPE(i);
    
    QLabel *label = new QLabel(vpe->getName());
    grid->addWidget(label, (i+1)*2, 0);
    label->setFont(labelFont);
    if (vpe->resource.get(CR_GPU_DEVICE)>0)
      label->setText(QString("<b><font color=\"maroon\">%1</font></b>").arg(label->text()));
    else
      label->setText(QString("<b><font color=\"darkGreen\">%1</font></b>").arg(label->text()));
    
    QVPEStatus *status = new QVPEStatus(vpe);    
    grid->addWidget(status, (i+1)*2, 1);
    status->setFont(statusFont);
    QEngineStatus::onAfterExecution(status);

    vpe->getBeforeExecutionCallback()->add(QEngineStatus::onBeforeExecution, status);
    vpe->getAfterExecutionCallback()->add(QEngineStatus::onAfterExecution, status);

    QFrame *hline = new QFrame();
    hline->setFrameStyle(QFrame::HLine | QFrame::Sunken);
    grid->addWidget(hline, (i+1)*2-1, 0, 1, 2);
  }

  grid->setColumnStretch(1, 1);
  grid->setRowStretch(this->engine->getNumberOfVPEs(), 1);
}

#define SetStatusEventType (QEvent::Type(QEvent::User+1))
class QSetStatusEvent: public QEvent {
public:
  QSetStatusEvent(const QString &str): QEvent(SetStatusEventType) {
    this->text = str;
  }
                                     
  QString text;
};

void QEngineStatus::onBeforeExecution(void *arg) {
  QVPEStatus *status = (QVPEStatus*)arg;
  RequestData *request = status->vpe->getCurrentRequest();
  QString str = QString("<center>%1</center>").arg(request->context.module->getName());
  QCoreApplication::postEvent(status, new QSetStatusEvent(str));
}

void QEngineStatus::onAfterExecution(void *arg) {
  QVPEStatus *status = (QVPEStatus*)arg;
  QString str = "<center><font color=\"red\">idle</font></center>";
  QCoreApplication::postEvent(status, new QSetStatusEvent(str));
}


QVPEStatus::QVPEStatus(VirtualProcessingElement *e) {
  this->vpe = e;
}

bool QVPEStatus::event(QEvent *e) { 
  if (e->type()==SetStatusEventType) {
    QSetStatusEvent *ste = (QSetStatusEvent*)e;
    this->setText(ste->text);
    this->setVisible(!ste->text.isEmpty());
    return true;
  }
  return QLabel::event(e);
}
