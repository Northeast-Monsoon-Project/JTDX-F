#ifndef JTDXF_MANAGED_MODE_HPP
#define JTDXF_MANAGED_MODE_HPP
#include <QString>
#include <QHash>
#include <QList>
class QSettings;

enum class ManagedMode { Off, DxccHunter, QsoEnthusiast };
enum ManagedNew { NewCqZone=1, NewDxcc=2, NewBandModeDxcc=4, NewItuZone=8,
                  NewGrid=16, NewCall=32, NewBandModeCall=64 };
struct ManagedProfile {
  ManagedProfile(unsigned flags=0,bool answer=false,int limit=3):criteria(flags),answerWorked(answer),callLimit(limit) {}
  unsigned criteria=0;
  bool answerWorked=false;
  int callLimit=3;
};
struct ManagedSettings {
  ManagedProfile hunter {NewCqZone|NewItuZone|NewDxcc|NewBandModeDxcc|NewGrid,false,3};
  ManagedProfile enthusiast {NewCall|NewBandModeCall,false,3};
  ManagedProfile profile(ManagedMode mode) const {return mode==ManagedMode::DxccHunter?hunter:enthusiast;}
  void read(QSettings&);
  void write(QSettings&) const;
};
struct ManagedCandidate {
  QString call,grid,mode,receivedReport,direction;
  int snr=-60,frequency=0;
  unsigned utcSeconds=0,novelty=0;
  qint64 seen=0;
  bool incoming=false,available=false,workedBandMode=false;
};

// All deadlines use a monotonic clock supplied by the caller. None of this
// session state (including retries and mode) is written to QSettings.
class ManagedSession {
public:
  ManagedMode mode() const {return mode_;}
  bool active() const {return mode_!=ManagedMode::Off;}
  void setMode(ManagedMode mode,qint64 now);
  void restartSchedule(qint64 now);
  void observe(ManagedCandidate const&);
  QList<ManagedCandidate> candidates(qint64 now,qint64 outgoingAge) const;
  ManagedCandidate choose(QList<ManagedCandidate> const&,ManagedProfile const&,qint64 now,bool incomingOnly=false) const;
  void begin(QString const& call);
  QString current() const {return current_;}
  void transmitted();
  bool exhausted(int limit) const;
  void failed(qint64 now);
  void complete(qint64 now);
  void clearCandidates();
  void cancelAttempt();
  bool blocked(QString const& call,qint64 now) const;
  bool cqDue(qint64 now);
  void cqTransmitted(qint64 now);
  int cqRemaining() const {return cqRemaining_;}
  static int rank(unsigned novelty);
private:
  struct Retry {int failures=0;qint64 until=0;};
  ManagedMode mode_=ManagedMode::Off;
  QHash<QString,ManagedCandidate> candidates_;
  QHash<QString,Retry> retries_;
  QHash<QString,qint64> completed_;
  QString current_;
  int transmissions_=0,cqRemaining_=0;
  bool replied_=false;
  qint64 nextCq_=0;
};
#endif
