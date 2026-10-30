#ifndef JTDXF_NTP_CONTROLLER_HPP
#define JTDXF_NTP_CONTROLLER_HPP
#include "NtpSettings.hpp"
#include <QTcpServer>
#include <QPointer>
#include <QTcpSocket>
#include <QProcess>
#include <QFutureWatcher>
#include <QTimer>
#include <QJsonObject>
class NtpController : public QObject {
  Q_OBJECT
public:
  explicit NtpController(QObject *parent=nullptr);
  ~NtpController();
  void authorize();
  void configure(NtpSettings const& settings);
  void stop();
  void setTransmitting(bool transmitting);
  bool authorized() const {return ready_;}
signals:
  void statusChanged(QString message);
  void authorizationChanged(bool ready);
  void synchronized(double offsetMs);
protected:
  virtual void launchHelper(QString const& path,QStringList const& args);
  void trackLaunch(QFuture<QString> const& future);
  virtual QJsonObject configurationMessage() const;
  void launchFailed(QString const& error);
private:
  QTcpServer listener_;QPointer<QTcpSocket> channel_;
  QFutureWatcher<QString> launch_;
  QTimer timeout_,heartbeat_;
  QString token_,launchToken_;NtpSettings settings_;
  bool ready_=false,pending_=false,transmitting_=false;
  bool authorizationWanted_=false,launchActive_=false;
  void send(QJsonObject const& data);
};
#endif
