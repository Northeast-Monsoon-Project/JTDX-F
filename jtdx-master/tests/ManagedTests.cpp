#include <QtTest>
#include <QTemporaryDir>
#include <QSettings>
#include <QCheckBox>
#include <QSpinBox>
#include "ManagedMode.hpp"
#include "ManagedSettings.hpp"

class ManagedTests : public QObject {
  Q_OBJECT
  ManagedCandidate station(QString call,unsigned flags,int snr=-10,bool incoming=false) {
    ManagedCandidate c;c.call=call;c.novelty=flags;c.snr=snr;c.incoming=incoming;c.available=!incoming;c.seen=1000;return c;
  }
private slots:
  void defaultsAndPersistence() {
    ManagedSession session;QVERIFY(!session.active());
    ManagedSettings defaults;QCOMPARE(defaults.hunter.callLimit,3);QVERIFY(!defaults.enthusiast.answerWorked);
    ManagedSettingsWidget widget;auto forced=widget.findChildren<QCheckBox*>(QRegularExpression(".*periodic_cq"));
    QCOMPARE(forced.size(),2);for(auto box:forced){QVERIFY(box->isChecked());QVERIFY(!box->isEnabled());}
    auto count=widget.findChild<QSpinBox*>("managed_hunter_call_limit");QVERIFY(count);QCOMPARE(count->minimum(),0);QCOMPARE(count->maximum(),99);
    defaults.hunter.criteria=NewGrid;defaults.hunter.callLimit=0;defaults.enthusiast.answerWorked=true;defaults.enthusiast.callLimit=99;
    widget.setSettings(defaults);auto selected=widget.settings();
    QTemporaryDir dir;QSettings stored(dir.path()+"/test.ini",QSettings::IniFormat);selected.write(stored);stored.sync();
    ManagedSettings loaded;loaded.read(stored);QCOMPARE(loaded.hunter.criteria,unsigned(NewGrid));QCOMPARE(loaded.hunter.callLimit,0);
    QVERIFY(loaded.enthusiast.answerWorked);QCOMPARE(loaded.enthusiast.callLimit,99);
    session.setMode(ManagedMode::DxccHunter,0);session.setMode(ManagedMode::QsoEnthusiast,1);
    ManagedSession reopened;QVERIFY(!reopened.active());QVERIFY(!stored.allKeys().contains("Mode"));
  }
  void eachCriterionIsIndependent() {
    ManagedSession session;session.setMode(ManagedMode::DxccHunter,0);
    for(unsigned flag:{NewCqZone,NewDxcc,NewBandModeDxcc,NewItuZone,NewGrid,NewCall,NewBandModeCall}) {
      ManagedProfile p(flag);auto wanted=station("K1AAA",flag),other=station("K2BBB",0,30);
      QCOMPARE(session.choose({other,wanted},p,1000).call,wanted.call);
      p.criteria=0;QVERIFY(session.choose({wanted},p,1000).call.isEmpty());
    }
  }
  void priorityAndSignalStrength() {
    ManagedSession session;session.setMode(ManagedMode::QsoEnthusiast,0);ManagedProfile p(127);
    QList<ManagedCandidate> stations;
    for(int i=0;i<7;++i)stations.append(station("K"+QString::number(i+1)+"AAA",1u<<i,i));
    for(int i=0;i<7;++i) {QCOMPARE(session.choose(stations,p,1000).call,stations.first().call);stations.removeFirst();}
    auto weak=station("K1AAA",NewCall,-25),strong=station("K2BBB",NewCall,5);
    QCOMPARE(session.choose({weak,strong},p,1000).call,strong.call);
    // Ranking uses all novelty, eligibility uses only this profile's criteria.
    weak.novelty|=NewCqZone;p.criteria=NewCall;QCOMPARE(session.choose({strong,weak},p,1000).call,weak.call);
  }
  void callersAndWorkedPolicy() {
    ManagedSession s;s.setMode(ManagedMode::DxccHunter,0);ManagedProfile p(NewDxcc);
    auto cq=station("K1AAA",NewDxcc,20),caller=station("K2BBB",0,-25,true);
    QCOMPARE(s.choose({cq,caller},p,1000).call,caller.call);
    caller.workedBandMode=true;QCOMPARE(s.choose({caller,cq},p,1000).call,cq.call);
    p.answerWorked=true;QCOMPARE(s.choose({caller,cq},p,1000).call,caller.call);
    s.begin(caller.call);s.failed(1000);QVERIFY(s.blocked(caller.call,1001));
    QCOMPARE(s.choose({caller,cq},p,1001).call,caller.call);
  }
  void attemptLimitsAndReplies() {
    ManagedSession s;s.setMode(ManagedMode::QsoEnthusiast,0);s.begin("K1AAA");
    for(int i=0;i<99;++i){QVERIFY(!s.exhausted(99));s.transmitted();}
    QVERIFY(s.exhausted(99));QVERIFY(!s.exhausted(0));
    auto reply=station("K1AAA",0,-10,true);s.observe(reply);QVERIFY(!s.exhausted(1));
    s.cancelAttempt();s.begin("K2BBB");QVERIFY(!s.exhausted(1));s.transmitted();QVERIFY(s.exhausted(1));
  }
  void retryEscalationAndRestart() {
    ManagedSession s;s.setMode(ManagedMode::QsoEnthusiast,0);
    s.begin("K1AAA");s.failed(1000);QVERIFY(s.blocked("K1AAA",600999));QVERIFY(!s.blocked("K1AAA",601000));
    s.begin("K1AAA/P");s.failed(601000);QVERIFY(s.blocked("K1AAA",2400999));QVERIFY(!s.blocked("K1AAA",2401000));
    s.begin("K1AAA");s.failed(2401000);QVERIFY(s.blocked("K1AAA",qint64(10000000000)));
    s.setMode(ManagedMode::Off,2401001);s.setMode(ManagedMode::QsoEnthusiast,2401002);QVERIFY(s.blocked("K1AAA",2401003));
    ManagedSession restarted;QVERIFY(!restarted.blocked("K1AAA",2401003));
    s.setMode(ManagedMode::DxccHunter,2401004);s.begin("K2BBB");s.failed(2401004);
    QVERIFY(s.blocked("K2BBB",3001003));QVERIFY(!s.blocked("K2BBB",3001004));
    s.begin("K2BBB");s.failed(3001004);QVERIFY(!s.blocked("K2BBB",3601004));
  }
  void freshReceptionRequiredAfterTimeout() {
    ManagedSession s;s.setMode(ManagedMode::QsoEnthusiast,0);auto cq=station("K1AAA",NewCall);s.observe(cq);
    s.begin(cq.call);s.failed(1000);QVERIFY(s.candidates(601000,32000).isEmpty());
    cq.seen=601001;s.observe(cq);QCOMPARE(s.candidates(601002,32000).size(),1);
    QVERIFY(s.candidates(634002,32000).isEmpty());
    cq.incoming=true;cq.seen=700000;s.observe(cq);QCOMPARE(s.candidates(800000,32000).size(),1);
    QVERIFY(s.candidates(820001,32000).isEmpty());
  }
  void completedCallDoesNotLoop() {
    ManagedSession s;s.setMode(ManagedMode::QsoEnthusiast,0);auto caller=station("K1AAA",0,-10,true);s.observe(caller);
    ManagedProfile p(0,true);s.begin(caller.call);s.complete(1001);
    QVERIFY(s.choose(s.candidates(1002,32000),p,1002).call.isEmpty());
    caller.seen=2000;s.observe(caller);QCOMPARE(s.choose(s.candidates(2000,32000),p,2000).call,caller.call);
  }
  void periodicCqThreeRounds() {
    ManagedSession s;QVERIFY(!s.cqDue(900000));s.setMode(ManagedMode::DxccHunter,1000);
    QVERIFY(!s.cqDue(900999));QVERIFY(s.cqDue(901000));QCOMPARE(s.cqRemaining(),3);
    s.cqTransmitted(901000);QCOMPARE(s.cqRemaining(),2);
    s.begin("K1AAA");s.transmitted();s.complete(920000);QCOMPARE(s.cqRemaining(),2);
    s.cqTransmitted(950000);s.cqTransmitted(980000);QCOMPARE(s.cqRemaining(),0);
    QVERIFY(!s.cqDue(1879999));QVERIFY(s.cqDue(1880000));
    s.restartSchedule(1880001);QCOMPARE(s.cqRemaining(),0);QVERIFY(!s.cqDue(1880002));
  }
};
QTEST_MAIN(ManagedTests)
#include "ManagedTests.moc"
