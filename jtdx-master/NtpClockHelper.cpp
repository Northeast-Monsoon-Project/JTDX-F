// Limited privileged process: NTP -> system clock only. No shell commands,
// file writes or arbitrary timestamps are accepted from the GUI connection.
#include "NtpClient.hpp"
#include "NtpInterval.hpp"
#include <QCoreApplication>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QElapsedTimer>
#include <QTextStream>
#ifdef Q_OS_WIN
#include <windows.h>
#else
#include <unistd.h>
#include <time.h>
#include <cerrno>
#include <cstring>
#endif

namespace {
bool clockPermission() {
#ifdef FM_NTP_FAKE_CLOCK
  return true;
#elif defined(Q_OS_WIN)
  HANDLE token=nullptr;
  if(!OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY,&token))return false;
  TOKEN_PRIVILEGES p{};p.PrivilegeCount=1;
  bool ok=LookupPrivilegeValueW(nullptr,L"SeSystemtimePrivilege",&p.Privileges[0].Luid);
  p.Privileges[0].Attributes=SE_PRIVILEGE_ENABLED;
  if(ok){SetLastError(ERROR_SUCCESS);ok=AdjustTokenPrivileges(token,FALSE,&p,0,nullptr,nullptr)&&GetLastError()==ERROR_SUCCESS;}
  CloseHandle(token);return ok;
#else
  return geteuid()==0;
#endif
}
QString setClock(qint64 ms) {
#ifdef FM_NTP_FAKE_CLOCK
  Q_UNUSED(ms);return {};
#elif defined(Q_OS_WIN)
  auto utc=QDateTime::fromMSecsSinceEpoch(ms,Qt::UTC);auto date=utc.date();auto time=utc.time();
  SYSTEMTIME t{};t.wYear=date.year();t.wMonth=date.month();t.wDay=date.day();t.wHour=time.hour();t.wMinute=time.minute();t.wSecond=time.second();t.wMilliseconds=time.msec();
  if(!SetSystemTime(&t))return QString("SetSystemTime: %1").arg(GetLastError());
  return {};
#else
  timespec t{};t.tv_sec=ms/1000;t.tv_nsec=(ms%1000)*1000000;
  if(clock_settime(CLOCK_REALTIME,&t)!=0)return QString::fromLocal8Bit(std::strerror(errno));
  return {};
#endif
}
}
int main(int argc,char **argv) {
  QCoreApplication app(argc,argv);
  auto args=app.arguments();if(args.size()!=3)return 2;
  // Read-only diagnostic path: never requests privilege or calls setClock.
  if(args[1]=="--query") {
    NtpClient probe;
    QObject::connect(&probe,&NtpClient::received,&app,[&](NtpSample sample){
      QTextStream(stdout)<<QJsonDocument(QJsonObject{{"utc",QDateTime::fromMSecsSinceEpoch(sample.utcMs,Qt::UTC).toString(Qt::ISODateWithMs)},{"offset_ms",sample.offsetMs},{"delay_ms",sample.delayMs}}).toJson(QJsonDocument::Compact)<<'\n';app.exit(0);
    });
    QObject::connect(&probe,&NtpClient::failed,&app,[&](QString error){QTextStream(stderr)<<error<<'\n';app.exit(1);});
    QTimer::singleShot(0,&probe,[&]{probe.query(args[2]);});
    return app.exec();
  }
  bool ok=false;auto port=args[1].toUShort(&ok);QString token=args[2];
  if(!ok || !port || token.size()!=64)return 2;
  bool permitted=clockPermission();
  QTcpSocket channel;NtpClient client;QTimer tick;QElapsedTimer alive,clock;
  clock.start();alive.start();QByteArray input;QString server;bool paused=true,configured=false,querying=false;
  qint64 due=0;quint16 ntpPort=123;int interval=15*60000;
  auto send=[&](QJsonObject data){channel.write(QJsonDocument(data).toJson(QJsonDocument::Compact)+'\n');channel.flush();};
  QObject::connect(&channel,&QTcpSocket::connected,&app,[&]{send({{"hello",token},{"ready",permitted}});if(!permitted)channel.disconnectFromHost();});
  QObject::connect(&channel,&QTcpSocket::disconnected,&app,&QCoreApplication::quit);
  QObject::connect(&channel,QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error),&app,[&]{app.quit();});
  QObject::connect(&channel,&QTcpSocket::readyRead,&app,[&]{
    input+=channel.readAll();if(input.size()>16384){app.quit();return;}
    while(input.contains('\n')) {
      auto end=input.indexOf('\n');auto doc=QJsonDocument::fromJson(input.left(end));input.remove(0,end+1);
      if(!doc.isObject()){app.quit();return;}auto obj=doc.object();alive.restart();
      if(obj.value("command")=="stop"){client.cancel();app.quit();return;}
      if(obj.contains("paused"))paused=obj.value("paused").toBool(true);
      if(obj.value("command")=="configure") {
        auto host=NtpClient::normalizeServer(obj.value("server").toString());
        if(host.isEmpty() || !permitted){app.quit();return;}
        if(!configured || host!=server){client.cancel();querying=false;server=host;due=0;}
        int requestedInterval=ntpIntervalMinutes(obj.value("interval_minutes").toInt(15))*60000;
        if(interval!=requestedInterval){interval=requestedInterval;due=0;}
        configured=true;
#ifdef FM_NTP_FAKE_CLOCK
        ntpPort=quint16(obj.value("test_port").toInt(123));
        interval=obj.value("test_interval_ms").toInt(interval);
#endif
      }
    }
  });
  QObject::connect(&client,&NtpClient::failed,&app,[&](QString error){querying=false;due=clock.elapsed()+60000;send({{"error",error}});});
  QObject::connect(&client,&NtpClient::received,&app,[&](NtpSample sample){
    querying=false;
    if(paused){due=clock.elapsed()+60000;send({{"deferred",true}});return;}
    QString error=setClock(sample.utcMs);
    if(error.isEmpty())send({{"synced",true},{"offset_ms",sample.offsetMs},{"delay_ms",sample.delayMs},{"utc_ms",QString::number(sample.utcMs)}});
    else send({{"error",error}});
    due=clock.elapsed()+(error.isEmpty()?interval:60000);
  });
  QObject::connect(&tick,&QTimer::timeout,&app,[&]{
    if(alive.elapsed()>30000){client.cancel();app.quit();return;}
    if(permitted && configured && !paused && !querying && clock.elapsed()>=due){querying=true;client.query(server,ntpPort);}
  });
  tick.start(250);channel.connectToHost(QHostAddress::LocalHost,port);
  return app.exec();
}
