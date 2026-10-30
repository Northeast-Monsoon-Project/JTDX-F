#include <QtTest>
#include <cmath>
#include <QNetworkDatagram>
#include <QComboBox>
#include <QLineEdit>
#include <QDir>
#include <QTimeZone>
#include <QtEndian>
#include <QtConcurrent/QtConcurrentRun>
#include "NtpClient.hpp"
#include "NtpController.hpp"

class TestController : public NtpController {
public:
  QProcess helper;quint16 port=123;int launches=0;bool deny=false,delayFirst=false;
  QJsonObject message() const {return configurationMessage();}
  ~TestController(){stop();helper.waitForFinished(5000);}
protected:
  void launchHelper(QString const&,QStringList const& args) override {
    ++launches;
    if(delayFirst && launches==1){
      trackLaunch(QtConcurrent::run([]{QThread::msleep(150);return QString("Old permission request denied");}));return;
    }
    if(deny){launchFailed("Denied by test permission provider");return;}
    helper.start(QDir(QCoreApplication::applicationDirPath()).filePath("fm_ntp_helper"
#ifdef Q_OS_WIN
    ".exe"
#endif
    ),args);
  }
  QJsonObject configurationMessage() const override {
    auto data=NtpController::configurationMessage();data.insert("test_port",port);data.insert("test_interval_ms",500);return data;
  }
};
class NtpTests : public QObject {
  Q_OBJECT
private slots:
  void initTestCase(){qRegisterMetaType<NtpSample>();}
  void serversAndControls() {
    auto presets=NtpSettingsWidget::presets();QVERIFY(presets.size()>=20);
    QSet<QString> continents,hosts;bool leftAsia=false;
    QMap<QString,int> providerCounts;
    QMap<QString,QSet<QString>> providers,countries,countryProviders;
    for(auto const& p:presets) {
      QVERIFY(!NtpClient::normalizeServer(p.host).isEmpty());QVERIFY(!hosts.contains(p.host));hosts.insert(p.host);continents.insert(p.continent);
      QVERIFY(!p.provider.isEmpty());QVERIFY(++providerCounts[p.provider]<=2);
      providers[p.continent].insert(p.provider);if(!p.country.isEmpty())countries[p.continent].insert(p.country);
      countryProviders[p.country].insert(p.provider);
      if(p.continent!="Asia")leftAsia=true;else QVERIFY(!leftAsia);
    }
    QVERIFY(continents.contains("Asia") && continents.contains("Europe") && continents.contains("Africa") && continents.contains("North America") && continents.contains("South America") && continents.contains("Oceania"));
    QVERIFY(providers["Europe"].size()>=5);QVERIFY(countries["Europe"].size()>=4);
    for(auto region:{"Africa","South America"}){QVERIFY(providers[region].size()>=5);QVERIFY(countries[region].size()>=3);}
    QVERIFY(countryProviders["JP"].size()>=3);QVERIFY(countryProviders["KR"].size()>=1);QVERIFY(countryProviders["AU"].size()>=1);
    for(auto host:{"ntp.ntsc.ac.cn","ntp1.baidu.com","cn.pool.ntp.org","ntp.tuna.tsinghua.edu.cn","time.windows.com","time.apple.com","time.asia.apple.com","time1.google.com","time2.google.com","time.cloudflare.com"})QVERIFY(hosts.contains(host));
    NtpSettingsWidget widget;widget.show();
    auto combo=widget.findChild<QComboBox*>("ntp_server");auto custom=widget.findChild<QLineEdit*>("ntp_custom");
    QVERIFY(combo);QVERIFY(custom);QVERIFY(!combo->isEnabled());QVERIFY(custom->isHidden());
    widget.setChecked(true);QVERIFY(combo->isEnabled());combo->setCurrentIndex(combo->count()-1);
    QVERIFY(!custom->isHidden());custom->setText("192.0.2.1");QVERIFY(widget.settings().custom);QCOMPARE(widget.settings().server,QString("192.0.2.1"));
    QTest::qWait(1);QVERIFY(!combo->geometry().intersects(custom->geometry()));
    auto saved=widget.settings();widget.setSettings(saved);QCOMPARE(widget.settings().server,saved.server);
    combo->setCurrentIndex(0);QVERIFY(custom->isHidden());widget.setChecked(false);QVERIFY(!combo->isEnabled());
    QVERIFY(NtpClient::normalizeServer("https://example.com/a").isEmpty());
    QVERIFY(NtpClient::normalizeServer("example.com;bad").isEmpty());
    QCOMPARE(NtpClient::normalizeServer("  NTP.Example.COM. "),QString("ntp.example.com"));
    QVERIFY(!NtpClient::normalizeServer("2001:db8::123").isEmpty());
  }
  void intervalSelectionAndRestore() {
    NtpSettings settings;QCOMPARE(settings.intervalMinutes,15);
    NtpSettingsWidget widget;widget.setSettings(settings);
    auto combo=widget.findChild<QComboBox*>("ntp_interval");QVERIFY(combo);QVERIFY(!combo->isEnabled());
    QCOMPARE(combo->count(),10);QCOMPARE(combo->currentData().toInt(),15);
    widget.setChecked(true);QVERIFY(combo->isEnabled());
    TestController controller;controller.deny=true;
    for(int minutes:QList<int>{1,5,10,15,30,60,180,360,540,720}) {
      combo->setCurrentIndex(combo->findData(minutes));auto saved=widget.settings();QCOMPARE(saved.intervalMinutes,minutes);
      NtpSettingsWidget reopened;reopened.setSettings(saved);QCOMPARE(reopened.settings().intervalMinutes,minutes);
      controller.configure(saved);QCOMPARE(controller.message().value("interval_minutes").toInt(),minutes);
      QCOMPARE(ntpIntervalMinutes(minutes),minutes);
    }
    settings.intervalMinutes=999;widget.setSettings(settings);QCOMPARE(widget.settings().intervalMinutes,15);
    QCOMPARE(ntpIntervalMinutes(-1),15);QCOMPARE(ntpIntervalMinutes(0),15);
    settings.server="retired-preset.example.org";settings.custom=false;widget.setSettings(settings);
    QVERIFY(widget.settings().custom);QCOMPARE(widget.settings().server,settings.server);
  }
  void protocolValidation_data() {QTest::addColumn<int>("year");QTest::newRow("current-era")<<2026;QTest::newRow("2036-wrap")<<2040;}
  void protocolValidation() {
    QFETCH(int,year);auto t=QDateTime(QDate(year,1,1),QTime(0,0),Qt::UTC).toMSecsSinceEpoch();auto id=NtpClient::timestamp(t);
    QByteArray p(48,0);p[0]=0x24;p[1]=2;p.replace(24,8,id);p.replace(32,8,NtpClient::timestamp(t+220));p.replace(40,8,NtpClient::timestamp(t+230));
    NtpSample sample;QString error;QVERIFY(NtpClient::decode(p,id,t,t+50,50,&sample,&error));
    QVERIFY(std::abs(sample.offsetMs-200)<1);QVERIFY(std::abs(sample.delayMs-40)<1);
    auto bad=p;bad[0]=char(0xe4);QVERIFY(!NtpClient::decode(bad,id,t,t+50,50,&sample,&error));
    bad=p;bad[1]=0;QVERIFY(!NtpClient::decode(bad,id,t,t+50,50,&sample,&error));
    bad=p;bad[24]=char(bad[24]^1);QVERIFY(!NtpClient::decode(bad,id,t,t+50,50,&sample,&error));
    QVERIFY(!NtpClient::decode(p.left(47),id,t,t+50,50,&sample,&error));
    QVERIFY(!NtpClient::decode(p,id,t,t+5000,50,&sample,&error));
    bad=p;bad.replace(40,8,QByteArray(8,0));QVERIFY(!NtpClient::decode(bad,id,t,t+50,50,&sample,&error));
  }
  void utcAcrossTimeZonesAndMidnight() {
    auto t=QDateTime(QDate(2026,3,31),QTime(23,59,59,900),Qt::UTC).toMSecsSinceEpoch();
    auto id=NtpClient::timestamp(t);QByteArray reply(48,0);reply[0]=0x24;reply[1]=2;
    reply.replace(24,8,id);reply.replace(32,8,NtpClient::timestamp(t+220));reply.replace(40,8,NtpClient::timestamp(t+230));
    NtpSample sample;QString error;QVERIFY(NtpClient::decode(reply,id,t,t+50,50,&sample,&error));
    auto utc=QDateTime::fromMSecsSinceEpoch(sample.utcMs,Qt::UTC);
    QCOMPARE(utc.date(),QDate(2026,4,1));QCOMPARE(utc.time().hour(),0);
    auto east=utc.toTimeZone(QTimeZone(8*3600));auto west=utc.toTimeZone(QTimeZone(-4*3600));
    QCOMPARE(east.time().hour(),8);QCOMPARE(east.date(),QDate(2026,4,1));
    QCOMPARE(west.time().hour(),20);QCOMPARE(west.date(),QDate(2026,3,31));
    QCOMPARE(east.toMSecsSinceEpoch(),sample.utcMs);QCOMPARE(west.toMSecsSinceEpoch(),sample.utcMs);
  }
  void permissionAndPeriodicSynchronization() {
    QUdpSocket server;QVERIFY(server.bind(QHostAddress(QHostAddress::LocalHost),quint16(0)));int requests=0;
    connect(&server,&QUdpSocket::readyRead,this,[&]{
      while(server.hasPendingDatagrams()){
        auto packet=server.receiveDatagram();auto data=packet.data();++requests;
        QByteArray reply(48,0);reply[0]=0x24;reply[1]=2;reply.replace(24,8,data.mid(40,8));
        auto now=QDateTime::currentMSecsSinceEpoch()+125;reply.replace(32,8,NtpClient::timestamp(now));reply.replace(40,8,NtpClient::timestamp(now));
        server.writeDatagram(reply,packet.senderAddress(),packet.senderPort());
      }
    });
    TestController controller;controller.port=server.localPort();NtpSettings settings;settings.enabled=true;settings.server="127.0.0.1";
    QSignalSpy synced(&controller,&NtpController::synchronized);
    auto before=QDateTime::currentMSecsSinceEpoch();QElapsedTimer elapsed;elapsed.start();
    controller.setTransmitting(true);controller.configure(settings);
    QTRY_VERIFY_WITH_TIMEOUT(controller.authorized(),10000);QCOMPARE(controller.launches,1);
    QTest::qWait(600);QCOMPARE(requests,0);
    controller.setTransmitting(false);QTRY_VERIFY_WITH_TIMEOUT(synced.size()>=2,5000);QVERIFY(requests>=2);
    QVERIFY(qAbs(QDateTime::currentMSecsSinceEpoch()-before-elapsed.elapsed())<100); // Fake helper never changes host clock.
    controller.stop();QTRY_COMPARE_WITH_TIMEOUT(controller.helper.state(),QProcess::NotRunning,5000);
    int stopped=requests;QTest::qWait(650);QCOMPARE(requests,stopped);QVERIFY(!controller.authorized());
    controller.configure(settings);QTRY_VERIFY_WITH_TIMEOUT(controller.authorized(),10000);QCOMPARE(controller.launches,2);
    controller.stop();QTRY_COMPARE_WITH_TIMEOUT(controller.helper.state(),QProcess::NotRunning,5000);
    controller.deny=true;controller.configure(settings);QVERIFY(!controller.authorized());QCOMPARE(controller.launches,3);
  }
  void reenableWhilePermissionPending() {
    TestController controller;controller.delayFirst=true;
    controller.authorize();QCOMPARE(controller.launches,1);
    controller.stop();controller.authorize();
    QTRY_VERIFY_WITH_TIMEOUT(controller.authorized(),10000);QCOMPARE(controller.launches,2);
    controller.stop();QTRY_COMPARE_WITH_TIMEOUT(controller.helper.state(),QProcess::NotRunning,5000);
    TestController cancelled;cancelled.delayFirst=true;cancelled.authorize();cancelled.stop();
    QTest::qWait(300);QCOMPARE(cancelled.launches,1);QVERIFY(!cancelled.authorized());
  }
};
QTEST_MAIN(NtpTests)
#include "NtpTests.moc"
