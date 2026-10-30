#include <QtTest>
#include <QApplication>
#include <QDialogButtonBox>
#include <QListWidget>
#include <QSharedMemory>
#include <QSettings>
#include <QTemporaryDir>
#include <QNetworkAccessManager>
#include <QProcessEnvironment>
#include <QBuffer>
#include <QDataStream>
#include <QTcpSocket>
#include <QTcpServer>
#include <future>
#include <thread>
#include "HRDTransceiver.hpp"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "about.h"
#include "getfile.h"
#include "McpServer.hpp"
#include "TxWatchdog.hpp"
#include "MetaDataRegistry.hpp"
#include "logbook/adif.h"
#include "OperatorLog.hpp"
#include "StationProfiles.hpp"
#include "NtpSettings.hpp"
#include "ManagedSettings.hpp"
#include "displaytext.h"
#include "decodedtext.h"
#include "soundin.h"
#include <QCryptographicHash>
#include <QLockFile>
extern dec_data_t dec_data;
extern qint32 g_iptt;

class RegressionTest : public QObject {
  Q_OBJECT
  QTemporaryDir temp;
  QSettings settings {temp.path()+"/test.ini", QSettings::IniFormat};
  QSharedMemory shared;
  QNetworkAccessManager network;
  std::unique_ptr<MainWindow> w;
  void manualEnable() {
    if (!w->ui->enableTxButton->isChecked()) QTest::mouseClick(w->ui->enableTxButton,Qt::LeftButton);
  }
  void stopWaveAudio() {
    // The File/Open action suspends live capture before loading a WAV.
    // These direct test calls must honor the same precondition and drain
    // queued capture callbacks before reusing the shared sample buffer.
    w->on_monitorButton_clicked(false);
    QMetaObject::invokeMethod(w->m_soundInput,[]{},Qt::BlockingQueuedConnection);
    QCoreApplication::processEvents();
    QVERIFY(!w->m_monitoring);
  }
  void saveManagedSettings(ManagedSettings const& selected,bool hide=false) {
    auto dialog=w->findChild<QDialog*>("configuration_dialog");QVERIFY(dialog);
    QTimer::singleShot(0,dialog,[dialog,selected,hide]{
      auto page=dialog->findChild<ManagedSettingsWidget*>("managed_tab");QVERIFY(page);page->setSettings(selected);
      dialog->findChild<QCheckBox*>("workedDontShow_check_box")->setChecked(hide);
      QMetaObject::invokeMethod(dialog,"accept",Qt::DirectConnection);
    });
    QCOMPARE(w->m_config.exec(),int(QDialog::Accepted));
    w->ui->decodedTextBrowser->setConfiguration(&w->m_config);
  }
  int managedDecode(QString const& message,QString stamp="000015",int snr=-10) {
    DecodedText decoded(stamp+QString(" %1  0.1 1700 ~ ").arg(snr,3)+message);
    w->observeManagedDecode(decoded);
    return w->ui->decodedTextBrowser->displayDecodedText(&decoded,"N0CALL",Radio::base_callsign(w->m_hisCall),w->m_hisGrid,false,w->m_logBook,w->m_qsoHistory,w->m_qsoHistory,14074000,"FT8",false,false,1500,{},{},{},{},false,w.get(),true);
  }
  void prepareManaged(ManagedMode mode) {
    w->on_stopTxButton_clicked();w->m_mode=w->m_modeTx="FT8";w->m_TRperiod=15;w->m_freqNominal=14074000;
    w->m_houndMode=false;w->m_autoseq=false;w->m_callMode=0;w->m_singleshot=true;w->m_autoTx=false;w->m_skipTx1=false;
    w->m_diskData=false;w->m_manualDecode=false;w->m_decoderBusy=false;
    w->m_qsoHistory.init();w->m_qsoHistory.owndata("NA","K","AA00",false);
    w->m_logBook.init("N0CALL","","","ZZ9QA");
    w->setManagedMode(mode);manualEnable();
  }
private slots:
  void initTestCase() {
    auto dialogGuard=new QTimer(this);
    connect(dialogGuard,&QTimer::timeout,this,[] {
      if (auto dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget())) {
        qWarning("Headless test dialog: %s %s",dialog->metaObject()->className(),qPrintable(dialog->windowTitle()));
        if (auto box=qobject_cast<QMessageBox*>(dialog)) qWarning("%s",qPrintable(box->text()));
        dialog->reject();
      }
    });
    dialogGuard->start(50);
    settings.setValue("Configuration/MyCall", "N0CALL");
    settings.setValue("Configuration/MyGrid", "AA00");
    settings.setValue("Configuration/PSKReporter", false);
    settings.setValue("Configuration/IDint",99);
    settings.setValue("Configuration/IDafter73",true);
    shared.setKey(QCoreApplication::applicationName());
    if (!shared.create(sizeof(dec_data))) {
      QVERIFY(shared.attach()); shared.detach();
      QVERIFY(shared.create(sizeof(dec_data)));
    }
    memset(shared.data(), 0, sizeof(dec_data));
    w.reset(new MainWindow(true, &settings, &shared, 4, &network, QProcessEnvironment::systemEnvironment()));
    w->m_guiTimer.stop(); w->minuteTimer.stop();
    w->on_monitorButton_clicked(false);
    w->m_autoTx = false;
    w->m_mode = w->m_modeTx = "FT8";
    w->m_TRperiod = 15;
  }
  void txRxSplitDefaultsAndUpgrade() {
    QVERIFY(!w->m_lockTxFreq);
    QVERIFY(!w->ui->pbTxLock->isChecked());
    w->ui->TxFreqSpinBox->setValue(1200);
    w->ui->RxFreqSpinBox->setValue(1700);
    QCOMPARE(w->ui->TxFreqSpinBox->value(),1200);
    w->ui->TxFreqSpinBox->setValue(1900);
    QCOMPARE(w->ui->RxFreqSpinBox->value(),1700);

    // An existing V3.1.0 configuration is corrected once.
    settings.remove("Common/TxRxSplitDefaultV311");
    settings.setValue("Common/LockTxFreq",true);
    w->readSettings();
    QVERIFY(!w->m_lockTxFreq);
    QVERIFY(!settings.value("Common/LockTxFreq").toBool());
    QVERIFY(settings.value("Common/TxRxSplitDefaultV311").toBool());

    // A later manual choice survives settings save/load.
    w->ui->pbTxLock->click();
    QVERIFY(w->m_lockTxFreq);
    w->writeSettings();
    w->m_lockTxFreq=false;
    w->readSettings();
    QVERIFY(w->m_lockTxFreq);
    w->ui->pbTxLock->click();
    QVERIFY(!w->m_lockTxFreq);
    w->writeSettings();
    w->readSettings();
    QVERIFY(!w->m_lockTxFreq);
  }
  void cwIdentificationDisabled() {
    QCOMPARE(w->m_config.id_interval(),0);QVERIFY(!w->m_config.id_after_73());
    auto dialog=w->findChild<QWidget*>("configuration_dialog");QVERIFY(dialog);
    QVERIFY(dialog->findChild<QSpinBox*>("CW_id_interval_spin_box")->isHidden());
    QVERIFY(dialog->findChild<QCheckBox*>("CW_id_after_73_check_box")->isHidden());
  }
  void logProcessingDialogAndResponsiveness() {
    QTRY_VERIFY_WITH_TIMEOUT(!w->m_logLoading && !w->m_logImportActive,15000);
    auto directory=OperatorLog::directory(w->m_config.my_callsign());
    QLockFile held(directory.filePath(".operator-log.lock"));QVERIFY(held.tryLock());
    QString source=temp.path()+"/processing.adi";QFile file(source);QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("<CALL:5>K9ZZZ<QSO_DATE:8>20260101<TIME_ON:6>123456<MODE:3>FT8<BAND:3>20m<EOR>\n");file.close();
    int ticks=0;QTimer timer;connect(&timer,&QTimer::timeout,this,[&]{++ticks;});timer.start(5);
    w->importOperatorLogs({source});QVERIFY(w->m_logProcessingDialog);QVERIFY(w->m_logProcessingDialog->isVisible());
    QTest::qWait(70);QVERIFY(ticks>3);
    auto output=qEnvironmentVariable("JTDX_FM_QA_DIR");if(!output.isEmpty()){QDir().mkpath(output);w->m_logProcessingDialog->grab().save(output+"/log-processing.png");}
    held.unlock();
    QTRY_VERIFY_WITH_TIMEOUT(!w->m_logImportActive && !w->m_logLoading && !w->m_logRefreshPending,20000);
    QVERIFY(!w->m_logProcessingDialog->isVisible());QVERIFY(file.exists());
    w->refreshOperatorLogs(true);QVERIFY(w->m_logProcessingDialog->isVisible());
    QTRY_VERIFY_WITH_TIMEOUT(!w->m_logLoading && !w->m_logRefreshPending,15000);QVERIFY(!w->m_logProcessingDialog->isVisible());
    QVERIFY(file.open(QIODevice::WriteOnly|QIODevice::Truncate));file.write("<CALL:999>broken");file.close();
    w->importOperatorLogs({source});QTRY_VERIFY_WITH_TIMEOUT(!w->m_logImportActive && !w->m_logLoading && !w->m_logRefreshPending,20000);
    QVERIFY(!w->m_logProcessingDialog->isVisible());QVERIFY(file.exists());
  }
  void manualTxGate() {
    w->on_stopTxButton_clicked();
    QVERIFY(!w->m_manualTxAuthorized);
    QSignalSpy ptt(&w->m_config,&Configuration::transceiver_ptt);
    w->enableTx_mode(true); w->ui->enableTxButton->click(); w->TxAgain();
    QVERIFY(!w->m_enableTx); QVERIFY(!w->m_manualTxAuthorized);
    QVERIFY(w->mcpCall("start_tx",{}).value("isError").toBool());
    w->startTx2();
    QVERIFY(!w->m_transmitting); QCOMPARE(g_iptt,0);
    for(auto const& signal:ptt) QVERIFY(!signal.at(0).toBool());
    manualEnable(); QVERIFY(w->m_manualTxAuthorized); QVERIFY(w->m_enableTx);
    auto selector=w->findChild<QComboBox*>("station_profile_selector");
    QVERIFY(selector); QVERIFY(!selector->isEnabled());
    w->TxAgainTimer.start(1); w->enableTxButtonTimer.start(1); w->ptt1Timer.start(1);
    w->on_stopTxButton_clicked();
    QVERIFY(!w->TxAgainTimer.isActive()); QVERIFY(!w->enableTxButtonTimer.isActive()); QVERIFY(!w->ptt1Timer.isActive());
    w->TxAgain(); w->startTx2(); w->on_tuneButton_clicked(true);
    QVERIFY(!w->m_enableTx); QVERIFY(!w->m_tune); QVERIFY(!w->m_manualTxAuthorized); QCOMPARE(g_iptt,0);
    QVERIFY(selector->isEnabled());
  }
  void profilesAndQuarterBoundaries() {
    StationProfile a; a.call=" n0call/qrp ";a.grid="AA00";
    QVERIFY(StationProfiles::validate(a).isEmpty());
    QCOMPARE(StationProfiles::folder(a.call),QString("N0CALL_QRP"));
    auto list=StationProfiles::decode(StationProfiles::encode({a,a})); QCOMPARE(list.size(),1);
    StationProfile b=a;b.call="N0CALL/P";
    QCOMPARE(StationProfiles::decode(StationProfiles::encode({a,b})).size(),2);
    a.call="../N0CALL"; QVERIFY(!StationProfiles::validate(a).isEmpty());
    a.call="COM1"; QVERIFY(!StationProfiles::validate(a).isEmpty());
    QDir base(temp.path()+"/quarters");
    for(auto date: {QDate(2025,12,31),QDate(2026,1,1),QDate(2026,3,31),QDate(2026,4,1),QDate(2026,7,1),QDate(2026,10,1)}) {
      QString path=OperatorLog::quarterFile("N0CALL/QRP",date,base);
      QCOMPARE(QFileInfo(path).fileName(),QString("N0CALL_QRP%1S%2.adi").arg(date.year()).arg((date.month()-1)/3+1));
      QVERIFY(QFileInfo(path).dir().exists());
    }
  }
  void profileManagerEditingKeepsLogs() {
    QDir base(temp.path()+"/manager");auto dir=OperatorLog::directory("N0CALL/P",base);
    QFile keep(dir.filePath("keep.adi"));QVERIFY(keep.open(QIODevice::WriteOnly));keep.write("private synthetic log marker");keep.close();
    StationProfile initial;initial.call="N0CALL";initial.grid="AA00";
    StationManager manager({initial});manager.show();
    auto edit=[&](QString call,QString grid,int action) {
      QTimer::singleShot(0,&manager,[&,call,grid] {
        auto dialog=manager.findChild<QDialog*>("station_editor");
        if(!dialog) return;
        dialog->findChild<QLineEdit*>("profile_call")->setText(call);
        dialog->findChild<QLineEdit*>("profile_grid")->setText(grid);
        dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Save)->click();
      });
      manager.findChild<QPushButton*>(QString("station_action_%1").arg(action))->click();
    };
    edit("n0call/p","FN20",0);QCOMPARE(manager.profiles().size(),2);QCOMPARE(manager.profiles()[1].call,QString("N0CALL/P"));
    edit("N0CALL/QRP","FN21",1);QCOMPARE(manager.profiles()[1].call,QString("N0CALL/QRP"));QCOMPARE(manager.profiles()[1].grid,QString("FN21"));
    manager.findChild<QPushButton*>("station_action_2")->click();QCOMPARE(manager.profiles().size(),1);
    QVERIFY(keep.exists());QCOMPARE(OperatorLog::directory("N0CALL/P",base).absolutePath(),dir.absolutePath());
  }
  void quarterlyImportPreservesAndDeduplicates() {
    QDir base(temp.path()+"/import");QString source=temp.path()+"/foreign.adi";
    OperatorLog::Record a{{"CALL","K1ABC"},{"STATION_CALLSIGN","N0CALL"},{"QSO_DATE","20260331"},{"TIME_ON","2359"},{"MODE","FT8"},{"BAND","20m"},{"COMMENT","line one\n<EOR> and <QSO_DATE:8>19990101 remain field text"}};
    auto b=a;b["QSO_DATE"]="20260401";b["CALL"]="K2ABC";
    auto c=b;c["STATION_CALLSIGN"]="N0CALL/QRP";
    QFile input(source);QVERIFY(input.open(QIODevice::WriteOnly));auto bytes=(OperatorLog::serialize(a)+OperatorLog::serialize(b)+OperatorLog::serialize(c)).toUtf8();input.write(bytes);input.close();
    auto imported=OperatorLog::importFiles("N0CALL",{source},base);QVERIFY2(imported.errors.isEmpty(),qPrintable(imported.errors.join(';')));QCOMPARE(imported.imported,3);QCOMPARE(imported.archived,1);
    QVERIFY(input.open(QIODevice::ReadOnly));QCOMPARE(input.readAll(),bytes);input.close();
    QFile archive(base.filePath("N0CALL/old/foreign.adi"));QVERIFY(archive.open(QIODevice::ReadOnly));QCOMPARE(archive.readAll(),bytes);
    QCOMPARE(OperatorLog::files("N0CALL",base).size(),2);QCOMPARE(OperatorLog::files("N0CALL/QRP",base).size(),1);
    auto retry=OperatorLog::importFiles("N0CALL",{source},base);QCOMPARE(retry.imported,0);QCOMPARE(retry.duplicates,3);
    QString error;auto records=OperatorLog::read(OperatorLog::quarterFile("N0CALL",QDate(2026,3,31),base),&error);QVERIFY(error.isEmpty());QCOMPARE(records.size(),1);QCOMPARE(records[0].value("COMMENT"),a.value("COMMENT"));
    CountryDat countries;ADIF adif;adif.init(OperatorLog::files("N0CALL",base),&countries);adif.load("N0CALL","","");
    QVERIFY(adif.match("K1ABC","20m","FT8"));QVERIFY(adif.match("K2ABC","20m","FT8"));QCOMPARE(adif.getCount("FT8"),2);
    // A malformed source must remain intact and must not create an archive.
    QString bad=base.filePath("N0CALL/bad.adi");QFile broken(bad);QVERIFY(broken.open(QIODevice::WriteOnly));broken.write("<CALL:999>K1ABC<EOR>");broken.close();
    auto failure=OperatorLog::importFiles("N0CALL",{bad},base);QVERIFY(!failure.errors.isEmpty());QVERIFY(broken.exists());QVERIFY(!QFile::exists(base.filePath("N0CALL/old/bad.adi")));
  }
  void legacyImportFailureIsRetryable() {
    QDir base(temp.path()+"/retry-import");
    OperatorLog::Record a{{"CALL","k1abc"},{"QSO_DATE","20260101"},{"TIME_ON","010203"},{"MODE","FT8"},{"FREQ","14.074"}};
    auto b=a;b["QSO_DATE"]="20260401";
    QString source=temp.path()+"/retry.adi";QFile file(source);QVERIFY(file.open(QIODevice::WriteOnly));file.write((OperatorLog::serialize(a)+OperatorLog::serialize(b)).toUtf8());file.close();
    QString blocked=OperatorLog::quarterFile("N0CALL",QDate(2026,4,1),base);QVERIFY(QDir().mkpath(blocked));
    auto failed=OperatorLog::importFiles("N0CALL",{source},base);QVERIFY(!failed.errors.isEmpty());QVERIFY(file.exists());QVERIFY(!QFile::exists(base.filePath("N0CALL/old/retry.adi")));
    QVERIFY(QDir().rmdir(blocked));auto retry=OperatorLog::importFiles("N0CALL",{source},base);QVERIFY(retry.errors.isEmpty());QCOMPARE(retry.imported,1);QCOMPARE(retry.duplicates,1);QCOMPARE(retry.archived,1);
    CountryDat countries;ADIF history;history.init(OperatorLog::files("N0CALL",base),&countries);history.load("N0CALL","","");
    QVERIFY(history.match("K1ABC","20m","FT8"));
    QString csv=temp.path()+"/legacy.log";QFile old(csv);QVERIFY(old.open(QIODevice::WriteOnly));old.write("2026-01-01,01:02:03,2026-01-01,01:03:04,K1ABC,FN20,14.074,FT8,-10,-11,5,Hello, world,Test Name\n");old.close();
    QString error;auto rows=OperatorLog::read(csv,&error);QVERIFY(error.isEmpty());QCOMPARE(rows.size(),1);QCOMPARE(rows[0].value("COMMENT"),QString("Hello, world"));QCOMPARE(rows[0].value("NAME"),QString("Test Name"));QCOMPARE(rows[0].value("TIME_OFF"),QString("010304"));
    auto converted=OperatorLog::importFiles("N0CALL",{csv},base);QCOMPARE(converted.imported,0);QCOMPARE(converted.duplicates,1);QVERIFY(converted.errors.isEmpty());
    // A file named old must not turn failed backup creation into source deletion.
    QDir blockedBase(temp.path()+"/blocked-archive");auto dir=OperatorLog::directory("N0CALL",blockedBase);
    QFile blocker(dir.filePath("old"));QVERIFY(blocker.open(QIODevice::WriteOnly));blocker.write("keep");blocker.close();
    QFile deposited(dir.filePath("import.adi"));QVERIFY(deposited.open(QIODevice::WriteOnly));auto original=OperatorLog::serialize(a).toUtf8();deposited.write(original);deposited.close();
    auto unarchived=OperatorLog::importFiles("N0CALL",{deposited.fileName()},blockedBase);QVERIFY(!unarchived.errors.isEmpty());
    QVERIFY(deposited.open(QIODevice::ReadOnly));QCOMPARE(deposited.readAll(),original);deposited.close();
  }
  void privateReferenceLog() {
    QString source=qEnvironmentVariable("JTDXF_PRIVATE_LOG");
    if(source.isEmpty()) QSKIP("Private user log is optional and is never distributed");
    QFile file(source);QVERIFY(file.open(QIODevice::ReadOnly));auto bytes=file.readAll();file.close();
    QString error;auto records=OperatorLog::read(source,&error);QVERIFY2(error.isEmpty(),qPrintable(error));QVERIFY(!records.isEmpty());
    QDir base(temp.path()+"/private-reference");auto result=OperatorLog::importFiles("N0CALL",{source},base);QVERIFY2(result.errors.isEmpty(),"Private reference import failed (see local validation, not source contents)");
    QCOMPARE(result.imported+result.duplicates,records.size());
    auto retry=OperatorLog::importFiles("N0CALL",{source},base);QVERIFY(retry.errors.isEmpty());QCOMPARE(retry.imported,0);QCOMPARE(retry.duplicates,records.size());
    int total=0,quarters=0;for(auto const& dir:base.entryList(QDir::Dirs|QDir::NoDotAndDotDot)) {
      for(auto const& path:OperatorLog::files(dir,base)) {++quarters;total+=OperatorLog::read(path,&error).size();QVERIFY(error.isEmpty());}
    }
    QCOMPARE(total,result.imported);QVERIFY(file.open(QIODevice::ReadOnly));QCOMPARE(QCryptographicHash::hash(file.readAll(),QCryptographicHash::Sha256),QCryptographicHash::hash(bytes,QCryptographicHash::Sha256));
    qInfo("Private reference: %d records, %d unique records, %d quarterly files, repeated import adds zero",records.size(),total,quarters);
  }
  void watchdogBoundaries() {
    TxWatchdog wd; auto t = TxWatchdog::Time{};
    wd.start(t);
    QVERIFY(!wd.expired(1, t+std::chrono::milliseconds(59999)));
    QVERIFY(wd.expired(1, t+std::chrono::milliseconds(60000)));
    QVERIFY(!wd.expired(0, t+std::chrono::hours(10000)));
    QVERIFY(!wd.expired(999, t+std::chrono::minutes(998)));
    QVERIFY(wd.expired(999, t+std::chrono::minutes(999)));
    wd.stop(); QVERIFY(!wd.expired(1, t+std::chrono::hours(10000)));
  }
  void activityDoesNotResetWatchdog() {
    w->m_config.set_watchdog(1);
    manualEnable();
    w->m_txSession.start(TxWatchdog::Clock::now()-std::chrono::seconds(65));
    w->txwatchdog(false); w->setRxFreq4(1500);
    QVERIFY(w->m_txSession.expired(1));
    w->txwatchdog(true);
    QVERIFY(!w->m_enableTx); QVERIFY(w->m_txwatchdog);
    w->txwatchdog(false); QVERIFY(w->m_txwatchdog);
    manualEnable(); QVERIFY(!w->m_txwatchdog); QVERIFY(!w->m_txSession.expired(1));
    w->enableTx_mode(false);
    w->m_config.set_watchdog(999); QCOMPARE(w->m_config.watchdog(), 999);
    w->m_config.set_watchdog(0);
  }
  void continuousQso_data() {
    QTest::addColumn<int>("status"); QTest::addColumn<bool>("incoming"); QTest::addColumn<bool>("cq"); QTest::addColumn<int>("rxTime");
    for (int s : {int(QsoHistory::SRR73), int(QsoHistory::S73), int(QsoHistory::FIN)}) {
      QTest::newRow(qPrintable(QString("incoming-%1").arg(s))) << s << true << false << 15;
      QTest::newRow(qPrintable(QString("cq-%1").arg(s))) << s << false << true << 15;
      QTest::newRow(qPrintable(QString("idle-%1").arg(s))) << s << false << false << 15;
      QTest::newRow(qPrintable(QString("cq-even-%1").arg(s))) << s << false << true << 30;
    }
  }
  void continuousQso() {
    QFETCH(int, status); QFETCH(bool, incoming); QFETCH(bool, cq); QFETCH(int, rxTime);
    w->m_qsoHistory.init(); w->m_qsoHistory.owndata("NA", "K", "AA00", false);
    w->m_autoseq=true; w->m_singleshot=false; w->m_houndMode=false; w->m_callMode=2;
    w->m_callPrioCQ=false; w->m_callWorkedB4=cq; w->m_answerWorkedB4=true; w->m_lockTxFreq=true;
    w->m_counter=0; w->m_txFirst=true; w->m_msDecStarted=15000; w->m_transmitting=false; g_iptt=0;
    w->ui->dxCallEntry->setText("K1AAA");
    w->m_qsoHistory.message("K1AAA", QsoHistory::Status(status), 0, "", "", "NA", "K", 0, "-10", 1500, "FT8");
    w->m_qsoHistory.time(rxTime);
    if (incoming) w->m_qsoHistory.message("K2BBB", QsoHistory::RCALL, 1, "FN20", "", "NA", "K", rxTime, "-12", 1600, "FT8");
    if (cq) w->m_qsoHistory.message("K3CCC", QsoHistory::RCQ, 1, "FN31", "", "NA", "K", rxTime, "-5", 1700, "FT8");
    manualEnable(); auto before=w->m_txSession.elapsedMs();
    w->process_Auto();
    QVERIFY(w->m_enableTx); QVERIFY(w->m_txSession.active()); QVERIFY(w->m_txSession.elapsedMs() >= before);
    QCOMPARE(w->m_hisCall, incoming ? QString("K2BBB") : cq ? QString("K3CCC") : QString());
    if (incoming || cq) QCOMPARE(w->m_txFirst, rxTime == 15);
    QCOMPARE(w->m_ntx, incoming ? 2 : cq ? (w->m_skipTx1 ? 2 : 1) : 6);
    w->enableTx_mode(false);
  }
  void singleQsoStops() {
    w->m_singleshot=true; w->m_qsoHistory.init();
    w->ui->dxCallEntry->setText("K1AAA");
    w->m_qsoHistory.message("K1AAA", QsoHistory::FIN, 0, "", "", "NA", "K", 0, "-10", 1500, "FT8");
    manualEnable(); w->process_Auto(); QVERIFY(!w->m_enableTx); w->m_singleshot=false;
  }
  void finalMessageIsNotInterrupted() {
    w->m_qsoHistory.init(); w->ui->dxCallEntry->setText("K1AAA");
    w->m_qsoHistory.message("K1AAA", QsoHistory::S73, 0, "", "", "NA", "K", 0, "-10", 1500, "FT8");
    w->m_transmitting=true; g_iptt=1; manualEnable();
    w->process_Auto(); QCOMPARE(w->m_hisCall, QString("K1AAA")); QVERIFY(w->m_enableTx);
    w->m_transmitting=false; g_iptt=0; w->enableTx_mode(false);
  }
  void historyPriorityAndMidnight() {
    QsoHistory h; h.init(); h.owndata("NA", "K", "AA00", false); h.time(15);
    h.message("K1AAA", QsoHistory::RCALL, 1, "FN20", "", "NA", "K", 86385, "-10", 1500, "FT8");
    h.message("K2BBB", QsoHistory::RCQ, 18, "FN20", "", "NA", "K", 15, "-1", 1500, "FT8");
    QString call,grid,rpt,mode; int rx=0,tx=0,count=0,prio=0; unsigned flags=1|64|128;
    QCOMPARE(h.autoseq(call,grid,rpt,rx,tx,flags,count,prio,mode), QsoHistory::RCALL);
    QCOMPARE(call,QString("K1AAA"));
    // Invalid locators must not overflow the fixed coordinate array.
    h.owndata("NA", "K", QString(10000,'A'), false);
    h.message("K3CCC", QsoHistory::RCALL, 1, "AA00AA00AAEXTRA", "", "NA", "K", 15, "-10", 1500, "FT8");
    h.init(); call.clear(); flags=1|64|128;
    QCOMPARE(h.autoseq(call,grid,rpt,rx,tx,flags,count,prio,mode),QsoHistory::NONE);
  }
  void searchCqAndFinalMessages_data() {
    QTest::addColumn<QString>("message");
    QTest::newRow("CQ") << QString("CQ K3CCC FN31");
    QTest::newRow("73") << QString("K9ZZZ K3CCC 73");
    QTest::newRow("RR73") << QString("K9ZZZ K3CCC RR73");
    QTest::newRow("RRR") << QString("K9ZZZ K3CCC RRR");
  }
  void searchCqAndFinalMessages() {
    QFETCH(QString,message);
    w->on_stopTxButton_clicked();w->clearDX("");
    w->m_qsoHistory.init();w->m_qsoHistory.owndata("NA","K","AA00",false);
    w->m_callMode=2;w->m_callPrioCQ=true;w->m_callWorkedB4=true;w->m_houndMode=false;
    w->m_counter=4; // Prior retry counter must not force a CQ before selecting.
    DecodedText decoded(QString("000015 -10  0.1 1700 ~ ")+message);
    w->ui->decodedTextBrowser->displayDecodedText(&decoded,"N0CALL","","",false,w->m_logBook,w->m_qsoHistory,w->m_qsoHistory,14074000,"FT8",true,true,1500,{},{},{},{},false,w.get());
    manualEnable();w->process_Auto();
    QCOMPARE(w->m_hisCall,QString("K3CCC"));
    QVERIFY(w->m_ntx==1 || (w->m_skipTx1 && w->m_ntx==2));
    QVERIFY(w->m_enableTx);w->on_stopTxButton_clicked();
  }
  void workedVisibilityDefaultsAndQsoMessages() {
    QVERIFY(!w->m_config.workedDontShow());
    QTRY_VERIFY(!w->m_logLoading && !w->m_logImportActive);
    saveManagedSettings(ManagedSettings{},true);QVERIFY(w->m_config.workedDontShow());
    w->m_logBook.init("N0CALL","","","ZZ9QA");w->m_logBook.addAsWorked("K1AAA","20m","FT8","20261030","FN31","");
    w->clearDX("");w->m_qsoHistory.init();
    QVERIFY(!(managedDecode("CQ K1AAA FN31")&2));
    QVERIFY(!(managedDecode("K9ZZZ K1AAA -10")&2));
    for(auto text:{"N0CALL K1AAA -10","N0CALL K1AAA R-10","N0CALL K1AAA RRR","N0CALL K1AAA RR73","N0CALL K1AAA 73"})QVERIFY(managedDecode(text)&2);
    w->ui->dxCallEntry->setText("K1AAA");QVERIFY(managedDecode("K9ZZZ K1AAA -10")&2);
    w->clearDX("");saveManagedSettings(ManagedSettings{},false);QVERIFY(managedDecode("CQ K1AAA FN31")&2);
  }
  void managedMenusAndManualGate() {
    QCOMPARE(w->m_managed.mode(),ManagedMode::Off);
    auto menus=w->ui->menuBar->actions();QCOMPARE(menus.indexOf(w->m_managedMenu->menuAction()),menus.indexOf(w->ui->menuAutoSeq->menuAction())+1);
    QCOMPARE(menus.indexOf(w->ui->menuDXpedition->menuAction()),menus.indexOf(w->m_managedMenu->menuAction())+1);
    w->setManagedMode(ManagedMode::DxccHunter);QVERIFY(!w->m_enableTx);QVERIFY(!w->m_manualTxAuthorized);
    w->enableTx_mode(true);QVERIFY(!w->m_enableTx);w->process_Auto();QVERIFY(!w->managedTxAllowed());QCOMPARE(g_iptt,0);
    w->setManagedMode(ManagedMode::QsoEnthusiast);QCOMPARE(w->m_managed.mode(),ManagedMode::QsoEnthusiast);
    int checked=0;for(auto action:w->m_managedMenu->actions())checked+=action->isChecked();QCOMPARE(checked,1);
    w->writeSettings();QVERIFY(!settings.contains("Common/ManagedMode"));
    w->setManagedMode(ManagedMode::Off);QVERIFY(w->m_managedOff->isChecked());QVERIFY(w->ui->menuAutoSeq->isEnabled());
    auto remote=w->mcpCall("set_options",{{"managed_mode","dxcc_hunter"}});QVERIFY(!remote.value("isError").toBool());
    QCOMPARE(w->mcpStatus().value("managed_mode").toString(),QString("dxcc_hunter"));QVERIFY(!w->m_manualTxAuthorized);QVERIFY(!w->m_enableTx);
    w->setManagedMode(ManagedMode::Off);
  }
  void managedCriteriaAndIncomingWorkedPolicy() {
    QTRY_VERIFY(!w->m_logLoading && !w->m_logImportActive);
    ManagedSettings config;config.hunter.criteria=0;saveManagedSettings(config,true);prepareManaged(ManagedMode::DxccHunter);
    managedDecode("CQ JA1AAA PM95");w->process_Auto();QVERIFY(w->m_hisCall.isEmpty());QVERIFY(!w->managedTxAllowed());
    managedDecode("N0CALL K2BBB FN31");w->process_Auto();QCOMPARE(w->m_hisCall,QString("K2BBB"));QVERIFY(w->managedTxAllowed());
    w->on_stopTxButton_clicked();config.hunter.criteria=NewDxcc;saveManagedSettings(config,true);prepareManaged(ManagedMode::DxccHunter);
    w->m_logBook.addAsWorked("K1AAA","20m","FT8","20261030","FN31","");
    managedDecode("N0CALL K1AAA FN31");managedDecode("CQ JA1AAA PM95");w->process_Auto();QCOMPARE(w->m_hisCall,QString("JA1AAA"));
    w->on_stopTxButton_clicked();config.hunter.answerWorked=true;saveManagedSettings(config,true);prepareManaged(ManagedMode::DxccHunter);
    w->m_logBook.addAsWorked("K1AAA","20m","FT8","20261030","FN31","");
    managedDecode("N0CALL K1AAA FN31");managedDecode("CQ JA1AAA PM95");w->process_Auto();QCOMPARE(w->m_hisCall,QString("K1AAA"));
    w->setManagedMode(ManagedMode::Off);saveManagedSettings(ManagedSettings{},false);
  }
  void managedQsoContinuationAndCallLimit() {
    QTRY_VERIFY(!w->m_logLoading && !w->m_logImportActive);prepareManaged(ManagedMode::QsoEnthusiast);
    managedDecode("CQ K3CCC FN31");w->process_Auto();QCOMPARE(w->m_hisCall,QString("K3CCC"));QCOMPARE(w->m_ntx,1);
    // Complete three real-send accounting events without asserting PTT.
    for(int i=0;i<3;++i){w->m_managedCallOnAir=true;w->managedTransmissionFinished();}
    w->process_Auto();QVERIFY(w->m_hisCall.isEmpty());QVERIFY(!w->managedTxAllowed());QVERIFY(w->m_managed.blocked("K3CCC",w->m_managedClock.elapsed()));
    managedDecode("CQ K3CCC FN31");w->process_Auto();QVERIFY(w->m_hisCall.isEmpty());
    managedDecode("CQ JA1AAA PM95");w->process_Auto();QCOMPARE(w->m_hisCall,QString("JA1AAA"));
    w->m_managedCallOnAir=true;w->managedTransmissionFinished();
    managedDecode("N0CALL JA1AAA -12","000045");w->process_Auto();QCOMPARE(w->m_ntx,3);
    for(int i=0;i<4;++i){w->m_managedCallOnAir=true;w->managedTransmissionFinished();}
    w->process_Auto();QCOMPARE(w->m_hisCall,QString("JA1AAA"));
    managedDecode("N0CALL K2BBB FN31","000045");
    w->m_qsoHistory.message("JA1AAA",QsoHistory::S73,0,"","","AS","JA",60,"-12",1500,"FT8");
    w->process_Auto();QCOMPARE(w->m_hisCall,QString("K2BBB"));QCOMPARE(w->m_ntx,2);QVERIFY(w->m_enableTx);
    w->m_qsoHistory.message("K2BBB",QsoHistory::S73,0,"","","NA","K",90,"-10",1500,"FT8");
    w->process_Auto();QVERIFY(w->m_hisCall.isEmpty());QVERIFY(!w->managedTxAllowed());QVERIFY(w->m_enableTx);
    w->on_stopTxButton_clicked();w->process_Auto();QVERIFY(!w->m_manualTxAuthorized);QVERIFY(!w->m_enableTx);QCOMPARE(g_iptt,0);
    w->setManagedMode(ManagedMode::Off);w->m_singleshot=false;
  }
  void managedNoveltyUsesOperatorBandAndModeHistory() {
    QTRY_VERIFY(!w->m_logLoading && !w->m_logImportActive);saveManagedSettings(ManagedSettings{});prepareManaged(ManagedMode::QsoEnthusiast);
    w->m_logBook.addAsWorked("K1AAA","20m","FT8","20261030","FN31","");
    ManagedCandidate c;c.call="K1AAA";c.grid="FN31";c.mode="FT8";c.available=true;c.seen=w->m_managedClock.elapsed();
    w->m_managed.observe(c);QVERIFY(w->chooseManagedCandidate().call.isEmpty());
    c.mode="FT4";w->m_managed.observe(c);auto selected=w->chooseManagedCandidate();QCOMPARE(selected.call,c.call);
    QVERIFY(selected.novelty&NewBandModeCall);QVERIFY(selected.novelty&NewBandModeDxcc);QVERIFY(!(selected.novelty&NewCall));QVERIFY(!(selected.novelty&NewDxcc));
    c.mode="FT8";w->m_freqNominal=7074000;w->m_managed.observe(c);selected=w->chooseManagedCandidate();QVERIFY(selected.novelty&NewBandModeCall);QVERIFY(selected.novelty&NewBandModeDxcc);
    w->m_freqNominal=14074000;c.call="JA1AAA";c.grid="PM95";w->m_managed.clearCandidates();w->m_managed.observe(c);selected=w->chooseManagedCandidate();
    QVERIFY(selected.novelty&NewCqZone);QVERIFY(selected.novelty&NewItuZone);QVERIFY(selected.novelty&NewDxcc);QVERIFY(selected.novelty&NewGrid);QVERIFY(selected.novelty&NewCall);
    w->setManagedMode(ManagedMode::Off);w->m_singleshot=false;
  }
  void managedFreshInvitationAfterPreviousQso() {
    QTRY_VERIFY(!w->m_logLoading && !w->m_logImportActive);
    ManagedSettings config;config.enthusiast.answerWorked=true;saveManagedSettings(config);prepareManaged(ManagedMode::QsoEnthusiast);
    w->m_logBook.addAsWorked("K4DDD","20m","FT8","20261030","FN31","");
    w->m_qsoHistory.message("K4DDD",QsoHistory::RCALL,0,"FN31","","NA","K",0,"-10",1500,"FT8");
    w->m_qsoHistory.complete("K4DDD");
    managedDecode("N0CALL K4DDD -12");w->process_Auto();QCOMPARE(w->m_hisCall,QString("K4DDD"));QCOMPARE(w->m_ntx,3);
    w->process_Auto();QCOMPARE(w->m_hisCall,QString("K4DDD"));QVERIFY(w->managedTxAllowed());
    w->setManagedMode(ManagedMode::Off);saveManagedSettings(ManagedSettings{});w->m_singleshot=false;
  }
  void managedPeriodicCqAndStop() {
    QTRY_VERIFY(!w->m_logLoading && !w->m_logImportActive);prepareManaged(ManagedMode::DxccHunter);
    w->process_Auto();QVERIFY(!w->managedTxAllowed());
    w->m_managed.setMode(ManagedMode::DxccHunter,w->m_managedClock.elapsed()-900000);
    w->process_Auto();QVERIFY(w->managedTxAllowed());QCOMPARE(w->m_ntx,6);QCOMPARE(w->m_managed.cqRemaining(),3);
    w->m_managedCqOnAir=true;w->managedTransmissionFinished();
    managedDecode("N0CALL K2BBB FN31");w->process_Auto();QCOMPARE(w->m_hisCall,QString("K2BBB"));QCOMPARE(w->m_managed.cqRemaining(),2);
    w->m_qsoHistory.message("K2BBB",QsoHistory::S73,0,"","","NA","K",30,"-10",1500,"FT8");
    w->process_Auto();QCOMPARE(w->m_ntx,6);QVERIFY(w->managedTxAllowed());
    for(int i=0;i<2;++i){w->m_managedCqOnAir=true;w->managedTransmissionFinished();w->process_Auto();}
    QVERIFY(!w->managedTxAllowed());QCOMPARE(w->m_managed.cqRemaining(),0);
    w->on_stopTxButton_clicked();w->m_managed.setMode(ManagedMode::DxccHunter,w->m_managedClock.elapsed()-900000);w->process_Auto();
    QVERIFY(!w->managedTxAllowed());w->startTx2();QCOMPARE(g_iptt,0);QVERIFY(!w->m_tx_when_ready);
    w->setManagedMode(ManagedMode::Off);w->m_singleshot=false;
  }
  void malformedAndUnicodeWave() {
    QString file = temp.path()+QString::fromUtf8("/测试_123456.wav");
    QByteArray bytes; QDataStream stream(&bytes,QIODevice::WriteOnly); stream.setByteOrder(QDataStream::LittleEndian);
    stream.writeRawData("RIFF",4); stream<<quint32(40); stream.writeRawData("WAVEfmt ",8);
    stream<<quint32(16)<<quint16(1)<<quint16(1)<<quint32(12000)<<quint32(24000)<<quint16(2)<<quint16(16);
    stream.writeRawData("data",4); stream<<quint32(4)<<qint16(123)<<qint16(-123);
    QFile f(file); QVERIFY(f.open(QIODevice::WriteOnly)); f.write(bytes); f.close();
    getfile(file,15); QCOMPARE(dec_data.params.kin,2); QCOMPARE(dec_data.d2[0],short(123)); QCOMPARE(dec_data.params.nutc,123456);
    QVERIFY(f.open(QIODevice::WriteOnly)); f.write("RIFF"); f.close();
    getfile(file,15); QCOMPARE(dec_data.params.kin,0); QCOMPARE(dec_data.params.newdat,0);
    bytes.replace(12,4,"junk"); QVERIFY(f.open(QIODevice::WriteOnly)); f.write(bytes); f.close();
    getfile(file,15); QCOMPARE(dec_data.params.kin,0);
  }
  void mcpProtocolAndPermissions() {
    int invoked=0;
    McpServer server([&invoked](QString const&, QJsonObject const&) { ++invoked; return McpServer::success({{"ok",true}}); });
    auto call=[&](QString name,QJsonObject args=QJsonObject{}) { return server.dispatch({{"jsonrpc","2.0"},{"id",1},{"method","tools/call"},{"params",QJsonObject{{"name",name},{"arguments",args}}}}); };
    QVERIFY(call("start_tx").value("result").toObject().value("isError").toBool()); QCOMPARE(invoked,0);
    QVERIFY(!call("stop_tx").value("result").toObject().value("isError").toBool()); QCOMPARE(invoked,1);
    QCOMPARE(call("set_options",{{"watchdog_minutes",1000}}).value("error").toObject().value("code").toInt(),-32602);
    QCOMPARE(call("set_options",{{"managed_mode","invalid"}}).value("error").toObject().value("code").toInt(),-32602);
    QCOMPARE(call("set_monitor",{{"enabled","yes"}}).value("error").toObject().value("code").toInt(),-32602);
    QVERIFY(server.dispatch({{"jsonrpc","2.0"},{"method","tools/call"},{"params",QJsonObject{{"name","stop_tx"}}}}).isEmpty()); QCOMPARE(invoked,1);
    auto init=server.dispatch({{"jsonrpc","2.0"},{"id",7},{"method","initialize"},{"params",QJsonObject{{"protocolVersion","2025-11-25"},{"capabilities",QJsonObject{}},{"clientInfo",QJsonObject{{"name","test"},{"version","1"}}}}}});
    QCOMPARE(init.value("result").toObject().value("protocolVersion").toString(),QString("2025-11-25"));
    QCOMPARE(init.value("result").toObject().value("serverInfo").toObject().value("version").toString(),QCoreApplication::applicationVersion());
    QCOMPARE(McpServer::tools().size(),21);
  }
  void mcpApplicationControls() {
    w->enableTx_mode(false); w->clearDX("");
    auto result=w->mcpCall("set_target",{{"callsign","K1ABC"},{"grid","FN20"},{"report",-12}});
    QVERIFY(!result.value("isError").toBool()); QCOMPARE(w->m_hisCall,QString("K1ABC")); QVERIFY(!w->m_enableTx);
    QVERIFY(w->mcpCall("set_target",{{"callsign","bad\nCALL"}}).value("isError").toBool());
    w->mcpCall("set_options",{{"watchdog_minutes",999},{"single_qso",false},{"selection","middle"}});
    QCOMPARE(w->m_config.watchdog(),999); QCOMPARE(w->m_callMode,2);
    QVERIFY(!w->mcpCall("get_config",{}).value("structuredContent").toObject().contains("token"));
    QVERIFY(w->mcpCall("get_log",{{"file","../foreign.adi"}}).value("isError").toBool());
    manualEnable();
    QVERIFY(w->mcpCall("set_frequency",{{"hz",14074000}}).value("isError").toBool());
    w->mcpCall("stop_tx",{}); QVERIFY(!w->m_enableTx);
  }
  void hrdFragmentedAndMalformedReplies() {
    for (int test=0; test<4; ++test) {
      QByteArray reply; QDataStream bytes(&reply,QIODevice::WriteOnly); bytes.setByteOrder(QDataStream::LittleEndian);
      bytes << quint32(test==3 ? 8 : 22) << quint32(test==1 ? 0 : 0x1234ABCD)
            << quint32(test==2 ? 0 : 0xABCD1234) << quint32(0) << quint16('O') << quint16('K') << quint16(0);
      std::promise<quint16> port; auto address=port.get_future();
      std::thread peer([&port, reply] {
        QTcpServer server; if(!server.listen(QHostAddress::LocalHost,0)) { port.set_value(0); return; }
        port.set_value(server.serverPort());
        if(!server.waitForNewConnection(3000)) return;
        auto sock=server.nextPendingConnection();
        if(!sock->bytesAvailable() && !sock->waitForReadyRead(3000)) return;
        sock->readAll();
        sock->write(reply.left(7)); sock->waitForBytesWritten(1000);
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
        sock->write(reply.mid(7)); sock->waitForBytesWritten(1000);
        sock->disconnectFromHost();
      });
      HRDTransceiver rig({}, "localhost", false, TransceiverFactory::TX_audio_source_front, 1000);
      rig.hrd_=new QTcpSocket(&rig); rig.protocol_=HRDTransceiver::v5;
      rig.hrd_->connectToHost(QHostAddress::LocalHost,address.get());
      bool connected=rig.hrd_->waitForConnected(3000), rejected=false; QString received;
      try { if(connected) received=rig.send_command("test",true,false); } catch (std::exception const&) { rejected=true; }
      peer.join();
      QVERIFY(connected);
      if(test==0) { QVERIFY(!rejected); QCOMPARE(received,QString("OK")); }
      else QVERIFY(rejected);
    }
  }
  void rigReconnectPreservesStopAndWave() {
    w->on_monitorButton_clicked(false);
    auto state=w->m_rigState;
    state.online(true); state.frequency(w->m_freqNominal);
    for (bool disk : {false,true}) {
      w->m_diskData=disk;
      w->m_rigState.online(false);
      w->handle_transceiver_update(state);
      QCOMPARE(w->m_diskData,disk);
      QVERIFY(!w->m_monitoring);
      QVERIFY(w->m_monitoroff);
    }
    w->m_diskData=false;
  }
  void realFt8WaveDecodes() {
    qInfo("FT8 test: selecting mode");
    w->enableTx_mode(false); w->m_autoseq=false; w->m_autoTx=false;
    w->on_actionFT8_triggered(); w->m_mcpDecodes=QJsonArray{};
    stopWaveAudio();
    qInfo("FT8 test: loading WAV");
    w->m_diskData=true; w->m_path=QCoreApplication::applicationDirPath()+"/test-ft8.wav";
    QVERIFY(QFile::exists(w->m_path));
    w->read_wav_file(w->m_path);
    auto decoded=[this] {
      for (auto const& d : w->m_mcpDecodes)
        if (d.toObject().value("raw").toString().contains("CQ K1ABC FN20")) return true;
      return false;
    };
    QElapsedTimer wait; wait.start();
    while (!decoded() && wait.elapsed()<20000) QTest::qWait(50);
    QVERIFY2(decoded(),qPrintable(QString("No fixture decode: disk=%1 busy=%2 monitor=%3 decodes=%4")
      .arg(w->m_diskData).arg(w->m_decoderBusy).arg(w->m_monitoring)
      .arg(QString::fromUtf8(QJsonDocument(w->m_mcpDecodes).toJson(QJsonDocument::Compact)))));
    QTRY_VERIFY_WITH_TIMEOUT(!w->m_decoderBusy,30000);
    QVERIFY(!w->m_enableTx);
    w->m_diskData=false;
  }
  void realSuperFoxWaveDecodes() {
    w->on_stopTxButton_clicked();w->m_autoseq=false;w->m_autoTx=false;
    w->ui->actionSuperFox->setChecked(true);
    QVERIFY(w->m_superFoxMode);QVERIFY(w->m_houndMode);QVERIFY(!w->m_houndTXfreqJumps);
    QCOMPARE(w->ui->RxFreqSpinBox->value(),750);
    stopWaveAudio();
    w->m_mcpDecodes=QJsonArray{};w->m_diskData=true;
    w->m_path=QCoreApplication::applicationDirPath()+"/test-superfox_000000.wav";
    QVERIFY(QFile::exists(w->m_path));w->read_wav_file(w->m_path);
    auto decoded=[this] {for(auto const& d:w->m_mcpDecodes) if(d.toObject().value("raw").toString().contains("W0AAA K1JT RR73")) return true;return false;};
    QElapsedTimer wait;wait.start();while(!decoded() && wait.elapsed()<30000) QTest::qWait(50);
    QVERIFY2(decoded(),qPrintable(QString("SuperFox decode missing; busy=%1 nsftol=%2 rx=%3 mode=%4 decodes=%5 stderr=%6")
      .arg(w->m_decoderBusy).arg(dec_data.params.nsftol).arg(dec_data.params.nfqso).arg(w->m_mode)
      .arg(QString::fromUtf8(QJsonDocument(w->m_mcpDecodes).toJson(QJsonDocument::Compact)))
      .arg(QString::fromUtf8(w->proc_jtdxjt9.readAllStandardError()))));
    QTRY_VERIFY_WITH_TIMEOUT(!w->m_decoderBusy,30000);QVERIFY(!w->m_enableTx);QCOMPARE(g_iptt,0);
    w->m_diskData=false;w->ui->actionSuperFox->setChecked(false);w->ui->actionEnable_hound_mode->setChecked(false);
  }
  void consecutiveWaveDecodeHandoffs() {
    w->on_stopTxButton_clicked(); w->m_autoseq=false; w->m_autoTx=false;
    w->on_actionFT8_triggered();
    stopWaveAudio();
    auto restarts=w->m_decoderRestarts;
    for (int cycle=0; cycle<6; ++cycle) {
      w->m_mcpDecodes=QJsonArray{}; w->m_diskData=true;
      w->m_path=QCoreApplication::applicationDirPath()+"/test-ft8.wav";
      w->read_wav_file(w->m_path);
      auto decoded=[this] {
        for (auto const& d:w->m_mcpDecodes)
          if(d.toObject().value("raw").toString().contains("CQ K1ABC FN20")) return true;
        return false;
      };
      QElapsedTimer deadline; deadline.start();
      while ((!decoded() || w->m_decoderBusy) && deadline.elapsed()<15000) {
        QCoreApplication::processEvents(QEventLoop::AllEvents,1);
        QThread::yieldCurrentThread();
      }
      QVERIFY2(decoded() && !w->m_decoderBusy,qPrintable(QString("Decode handoff failed at cycle %1: busy=%2 wav=%3 lock=%4 newdat=%5 kin=%6 decodes=%7 stderr=%8")
        .arg(cycle).arg(w->m_decoderBusy).arg(w->m_wav_future_watcher.isRunning())
        .arg(QFile::exists(w->m_config.temp_dir().absoluteFilePath(".lock")))
        .arg(dec_data.params.newdat).arg(dec_data.params.kin)
        .arg(QString::fromUtf8(QJsonDocument(w->m_mcpDecodes).toJson(QJsonDocument::Compact)))
        .arg(QString::fromUtf8(w->proc_jtdxjt9.readAllStandardError()))));
      QCOMPARE(w->m_decoderRestarts,restarts);
      QVERIFY(!w->m_enableTx); QCOMPARE(g_iptt,0);
      // Submit the next file immediately: no artificial acknowledgement delay.
    }
    w->m_diskData=false;
  }
  void decoderFailureDoesNotCloseApplication() {
    QTRY_COMPARE(w->proc_jtdxjt9.state(),QProcess::Running);
    w->m_autoTx=false; manualEnable();
    w->proc_jtdxjt9.kill();
    QTRY_VERIFY(w->m_decoderRestarts > 0);
    QTRY_VERIFY(!w->m_decoderRecovering);
    QTRY_COMPARE(w->proc_jtdxjt9.state(),QProcess::Running);
    QVERIFY(w->m_valid); QVERIFY(!w->m_enableTx); QVERIFY(!w->m_decoderBusy); QVERIFY(!w->m_monitoring);
  }
  void macroModelIsReused() {
    w->ui->dxCallEntry->setText("K1AAA"); auto model=w->m_generatedMacros;
    w->ui->dxCallEntry->setText("K2BBB"); QCOMPARE(w->m_generatedMacros,model);
  }
  void snapshots() {
    QString output=qEnvironmentVariable("JTDX_FM_QA_DIR"); if(output.isEmpty()) return;
    QDir().mkpath(output); w->show();
    w->grab().save(output+"/main.png");
    CAboutDlg about; about.show(); about.grab().save(output+"/about.png");
    QWidget *dialog=w->m_config.findChild<QWidget*>("configuration_dialog");
    // The impl dialog is parented to MainWindow, not Configuration.
    if (!dialog) dialog=w->findChild<QWidget*>("configuration_dialog");
    QVERIFY(dialog);
    auto tabs=dialog->findChild<QTabWidget*>("configuration_tabs"); QVERIFY(tabs);
    auto managed=dialog->findChild<ManagedSettingsWidget*>("managed_tab");QVERIFY(managed);
    auto managedScroll=qobject_cast<QScrollArea*>(managed->parentWidget()->parentWidget());QVERIFY(managedScroll);
    QCOMPARE(tabs->indexOf(managedScroll),tabs->indexOf(dialog->findChild<QWidget*>("filters_tab"))+1);
    QCOMPARE(tabs->indexOf(dialog->findChild<QWidget*>("scheduler_tab")),tabs->indexOf(managedScroll)+1);
    tabs->setCurrentWidget(managedScroll);dialog->show();dialog->grab().save(output+"/managed-settings.png");
    auto page=dialog->findChild<QWidget*>("mcp_tab"); QVERIFY(page);
    tabs->setCurrentWidget(page); dialog->show(); dialog->grab().save(output+"/mcp-settings.png");
    tabs->setCurrentIndex(0);dialog->grab().save(output+"/station-settings.png");
    auto ntp=dialog->findChild<NtpSettingsWidget*>("ntp_group");QVERIFY(ntp);
    auto previous=ntp->settings();NtpSettings preview;preview.enabled=true;preview.custom=true;preview.server="192.0.2.1";
    ntp->setSettings(preview); // Signal-blocked layout preview, never requests real privileges.
    auto status=ntp->findChild<QLabel*>("ntp_status");auto previousStatus=status->text();status->setText(QString());
    ntp->layout()->activate();
    dialog->grab().save(output+"/ntp-custom-settings.png");ntp->setSettings(previous);status->setText(previousStatus);dialog->hide();
    StationProfile profile;profile.call="N0CALL";profile.grid="AA00";
    StationManager manager({profile},w.get());manager.show();manager.grab().save(output+"/station-manager.png");
  }
  void ntpIntervalPersistsAcrossRestarts() {
    // Configuration owns process-wide Hamlib resources; test restart after closing the main window.
    w->close();w.reset();JTDXDateTime testClock;
    QSettings stored(temp.path()+"/ntp-settings.ini",QSettings::IniFormat);
    stored.setValue("Configuration/MyCall","N0CALL");stored.setValue("Configuration/MyGrid","AA00");
    stored.setValue("Configuration/workedDontShow",true); // Old uninitialized default, corrected once.
    {
      QWidget owner;Configuration config(&stored,&owner);
      config.set_jtdxtime(&testClock);
      QVERIFY(!config.workedDontShow());QVERIFY(stored.value("Configuration/WorkedVisibilityDefaultV330").toBool());
      auto dialog=owner.findChild<QDialog*>("configuration_dialog");QVERIFY(dialog);
      auto ntp=dialog->findChild<NtpSettingsWidget*>("ntp_group");QVERIFY(ntp);
      QCOMPARE(ntp->settings().intervalMinutes,15);QVERIFY(!ntp->settings().enabled);
      QTimer::singleShot(0,dialog,[dialog,ntp]{
        auto selected=ntp->settings();selected.intervalMinutes=720;ntp->setSettings(selected);
        dialog->findChild<QCheckBox*>("workedDontShow_check_box")->setChecked(true);
        auto managed=dialog->findChild<ManagedSettingsWidget*>("managed_tab");
        auto profile=managed->settings();profile.hunter.criteria=NewGrid;profile.hunter.callLimit=0;profile.enthusiast.answerWorked=true;profile.enthusiast.callLimit=99;managed->setSettings(profile);
        QMetaObject::invokeMethod(dialog,"accept",Qt::DirectConnection);
      });
      QCOMPARE(config.exec(),int(QDialog::Accepted));
    }
    stored.sync();QCOMPARE(stored.value("Configuration/NTP/IntervalMinutes").toInt(),720);
    {
      QWidget owner;Configuration reopened(&stored,&owner);
      reopened.set_jtdxtime(&testClock);
      QVERIFY(reopened.workedDontShow()); // A deliberate new choice is preserved.
      auto ntp=owner.findChild<NtpSettingsWidget*>("ntp_group");QVERIFY(ntp);
      QCOMPARE(ntp->settings().intervalMinutes,720);QVERIFY(!ntp->settings().enabled);
      QCOMPARE(reopened.managed_settings().hunter.criteria,unsigned(NewGrid));QCOMPARE(reopened.managed_settings().hunter.callLimit,0);
      QVERIFY(reopened.managed_settings().enthusiast.answerWorked);QCOMPARE(reopened.managed_settings().enthusiast.callLimit,99);
    }
  }
  void cleanupTestCase() { if(w) w->close(); w.reset(); }
};
int main(int argc,char** argv) {
  register_types();
  QApplication app(argc,argv);
  QCoreApplication::setApplicationName("JTDX-F-regression");
  QCoreApplication::setApplicationVersion("3.3.0");
  QCoreApplication::setOrganizationName("Fluxmancer");
  QTranslator translation;
  if (qEnvironmentVariable("JTDX_FM_QA_LANG") == "zh_CN") {
    translation.load(":/Translations/jtdx_zh_CN.qm");
    app.installTranslator(&translation);
  }
  RegressionTest test;
  return QTest::qExec(&test,argc,argv);
}
#include "Regression.moc"
