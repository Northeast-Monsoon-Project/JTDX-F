#ifndef JTDXF_NTP_CLIENT_HPP
#define JTDXF_NTP_CLIENT_HPP
#include <QObject>
#include <QUdpSocket>
#include <QTimer>
#include <QElapsedTimer>

struct NtpSample { qint64 utcMs; double offsetMs,delayMs; NtpSample(qint64 utc=0,double offset=0,double delay=0):utcMs(utc),offsetMs(offset),delayMs(delay) {} };
Q_DECLARE_METATYPE(NtpSample)

class NtpClient : public QObject {
  Q_OBJECT
public:
  explicit NtpClient(QObject *parent=nullptr);
  static QString normalizeServer(QString const& server);
  static QByteArray timestamp(qint64 unixMs);
  static bool decode(QByteArray const& packet,QByteArray const& requestId,
                     qint64 sentMs,qint64 receivedMs,qint64 elapsedMs,
                     NtpSample *sample,QString *error);
  void query(QString const& host,quint16 port=123);
  void cancel();
signals:
  void received(NtpSample sample);
  void failed(QString error);
private:
  QUdpSocket socket_;
  QTimer timeout_;
  QElapsedTimer elapsed_;
  QByteArray requestId_;
  qint64 sentMs_=0;
  bool running_=false;
  void fail(QString const& error);
};
#endif
