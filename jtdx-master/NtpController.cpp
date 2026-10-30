#include "NtpController.hpp"
#include "NtpClient.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QRandomGenerator>
#include <QStandardPaths>
#include <QtConcurrent/QtConcurrentRun>
#include <memory>
#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#endif

NtpController::NtpController(QObject *parent):QObject(parent) {
  timeout_.setSingleShot(true);heartbeat_.setInterval(2000);
  connect(&timeout_,&QTimer::timeout,this,[this]{launchFailed(tr("Permission request timed out. System time is unchanged."));});
  connect(&heartbeat_,&QTimer::timeout,this,[this]{send({{"command","heartbeat"},{"paused",transmitting_}});});
  connect(&launch_,&QFutureWatcher<QString>::finished,this,[this]{
    launchActive_=false;
    if(pending_ && token_==launchToken_ && !launch_.result().isEmpty())launchFailed(launch_.result());
    // A checkbox can be disabled and enabled again while Windows still shows
    // the previous UAC prompt. Its result belongs only to that old session.
    if(authorizationWanted_ && !pending_ && !ready_)authorize();
  });
  connect(&listener_,&QTcpServer::newConnection,this,[this]{
    while(listener_.hasPendingConnections()) {
      auto socket=listener_.nextPendingConnection();socket->setReadBufferSize(16385);
      auto buffer=std::make_shared<QByteArray>();
      connect(socket,&QTcpSocket::disconnected,socket,&QObject::deleteLater);
      QTimer::singleShot(5000,socket,[this,socket]{if(channel_!=socket)socket->abort();});
      connect(socket,&QTcpSocket::readyRead,this,[this,socket,buffer]{
        *buffer+=socket->readAll();if(buffer->size()>16384){socket->abort();return;}
        while(buffer->contains('\n')) {
          int end=buffer->indexOf('\n');auto doc=QJsonDocument::fromJson(buffer->left(end));buffer->remove(0,end+1);
          if(!doc.isObject()){socket->abort();return;}auto data=doc.object();
          if(channel_!=socket) {
            if(!pending_ || data.value("hello").toString()!=token_){socket->abort();return;}
            if(!data.value("ready").toBool()){launchFailed(tr("Permission was not granted. Using system time."));socket->abort();return;}
            channel_=socket;pending_=false;ready_=true;timeout_.stop();listener_.close();heartbeat_.start();
            emit authorizationChanged(true);emit statusChanged(tr("Permission granted. Synchronization follows the selected interval."));
            if(settings_.enabled)send(configurationMessage());
            connect(socket,&QTcpSocket::disconnected,this,[this,socket]{if(channel_==socket){channel_.clear();ready_=false;heartbeat_.stop();emit authorizationChanged(false);emit statusChanged(tr("Time synchronization stopped. Using system time."));}});
          } else if(data.value("synced").toBool()) {
            auto offset=data.value("offset_ms").toDouble();
            emit statusChanged(tr("Time synchronized; correction %1 ms. Next check in %2 minutes.").arg(offset,0,'f',1).arg(settings_.intervalMinutes));emit synchronized(offset);
          } else if(data.contains("error"))emit statusChanged(tr("Synchronization failed: %1. Retrying in 1 minute.").arg(QCoreApplication::translate("NtpClient",data.value("error").toString().toUtf8().constData())));
          else if(data.value("deferred").toBool())emit statusChanged(tr("Synchronization deferred while transmitting."));
        }
      });
    }
  });
}
NtpController::~NtpController(){stop();}
void NtpController::authorize() {
  authorizationWanted_=true;
  if(ready_ || pending_ || launchActive_)return;
  if(!listener_.listen(QHostAddress::LocalHost,0)){emit statusChanged(listener_.errorString());return;}
  token_.clear();for(int n=0;n<8;++n)token_+=QString("%1").arg(QRandomGenerator::system()->generate(),8,16,QChar('0'));
  pending_=true;timeout_.start(120000);emit statusChanged(tr("Requesting permission to set the system time…"));
  auto path=QDir(QCoreApplication::applicationDirPath()).filePath("jtdx-f-clock"
#ifdef Q_OS_WIN
  ".exe"
#endif
  );
  launchHelper(path,{QString::number(listener_.serverPort()),token_});
}
void NtpController::configure(NtpSettings const& settings) {
  settings_=settings;
  settings_.intervalMinutes=ntpIntervalMinutes(settings.intervalMinutes);
  if(!settings.enabled){stop();return;}
  if(NtpClient::normalizeServer(settings.server).isEmpty()){stop();emit statusChanged(tr("Enter a valid NTP hostname or IP address."));return;}
  if(ready_)send(configurationMessage());else authorize();
}
QJsonObject NtpController::configurationMessage() const{return {{"command","configure"},{"server",settings_.server},{"paused",transmitting_},{"interval_minutes",settings_.intervalMinutes}};}
void NtpController::setTransmitting(bool active){transmitting_=active;if(ready_)send({{"command","heartbeat"},{"paused",active}});}
void NtpController::stop() {
  authorizationWanted_=false;pending_=false;ready_=false;token_.clear();timeout_.stop();heartbeat_.stop();listener_.close();
  if(channel_){auto socket=channel_.data();channel_.clear();socket->write("{\"command\":\"stop\"}\n");socket->flush();socket->disconnectFromHost();}
  emit authorizationChanged(false);emit statusChanged(tr("Use system time. NTP synchronization is disabled."));
}
void NtpController::send(QJsonObject const& data){if(channel_)channel_->write(QJsonDocument(data).toJson(QJsonDocument::Compact)+'\n');}
void NtpController::launchFailed(QString const& error){stop();emit statusChanged(error);}
void NtpController::trackLaunch(QFuture<QString> const& future) {
  launchActive_=true;launchToken_=token_;launch_.setFuture(future);
}
void NtpController::launchHelper(QString const& path,QStringList const& args) {
  if(!QFileInfo::exists(path)){launchFailed(tr("The time synchronization helper is missing."));return;}
#ifdef Q_OS_WIN
  trackLaunch(QtConcurrent::run([path,args]{
    auto com=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED|COINIT_DISABLE_OLE1DDE);
    auto executable=QDir::toNativeSeparators(path).toStdWString();auto parameters=args.join(' ').toStdWString();
    SHELLEXECUTEINFOW info{};info.cbSize=sizeof(info);info.fMask=SEE_MASK_NOCLOSEPROCESS|SEE_MASK_NOASYNC;info.lpVerb=L"runas";info.lpFile=executable.c_str();info.lpParameters=parameters.c_str();info.nShow=SW_HIDE;
    QString error;
    if(!ShellExecuteExW(&info))error=QCoreApplication::translate("NtpController","Permission was not granted (Windows error %1). Using system time.").arg(GetLastError());
    if(info.hProcess)CloseHandle(info.hProcess);
    if(SUCCEEDED(com))CoUninitialize();
    return error;
  }));
#else
  auto pkexec=QStandardPaths::findExecutable("pkexec");
  if(pkexec.isEmpty()){launchFailed(tr("Install polkit/pkexec to authorize system time synchronization."));return;}
  auto process=new QProcess(this);auto session=token_;
  connect(process,&QProcess::errorOccurred,this,[this,process,session]{if(pending_ && token_==session)launchFailed(process->errorString());});
  connect(process,QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished),this,[this,process,session](int,QProcess::ExitStatus){
    if(pending_ && token_==session)launchFailed(tr("Permission was not granted. Using system time."));
    process->deleteLater();
  });
  process->start(pkexec,QStringList{path}+args);
#endif
}
