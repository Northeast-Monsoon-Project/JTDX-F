#include "ProcessingDialog.hpp"
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QCloseEvent>
ProcessingDialog::ProcessingDialog(QWidget *parent):QDialog(parent) {
  setObjectName("log_processing_dialog");setWindowTitle(tr("Processing"));
  setWindowFlags(Qt::Dialog|Qt::CustomizeWindowHint|Qt::WindowTitleHint);
  setWindowModality(Qt::WindowModal);setMinimumWidth(320);
  auto layout=new QVBoxLayout(this);layout->setContentsMargins(24,22,24,22);
  auto message=new QLabel(tr("System processing, please wait…"),this);message->setAlignment(Qt::AlignCenter);layout->addWidget(message);
  auto bar=new QProgressBar(this);bar->setRange(0,0);bar->setTextVisible(false);layout->addWidget(bar);
  stop_=new QPushButton(tr("Stop TX"),this);stop_->setObjectName("processing_stop_tx");layout->addWidget(stop_);stop_->hide();
  connect(stop_,&QPushButton::clicked,this,&ProcessingDialog::stopTxRequested);
}
void ProcessingDialog::setStopVisible(bool visible){stop_->setVisible(visible);}
void ProcessingDialog::closeEvent(QCloseEvent *event){event->ignore();}
