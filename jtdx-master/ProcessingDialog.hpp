#ifndef JTDXF_PROCESSING_DIALOG_HPP
#define JTDXF_PROCESSING_DIALOG_HPP
#include <QDialog>
class QPushButton;
class ProcessingDialog : public QDialog {
  Q_OBJECT
public:
  explicit ProcessingDialog(QWidget *parent=nullptr);
  void setStopVisible(bool visible);
signals:
  void stopTxRequested();
protected:
  void reject() override {}
  void closeEvent(QCloseEvent *event) override;
private:
  QPushButton *stop_;
};
#endif
