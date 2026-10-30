#include "ManagedMode.hpp"
#include "Radio.hpp"
#include <QSettings>
#include <limits>

void ManagedSettings::read(QSettings& settings) {
  auto read=[&](QString name,ManagedProfile& p,unsigned allowed) {
    settings.beginGroup("Managed/"+name);
    p.criteria=settings.value("Criteria",p.criteria).toUInt()&allowed;
    p.answerWorked=settings.value("AnswerWorked",false).toBool();
    p.callLimit=qBound(0,settings.value("CallLimit",3).toInt(),99);
    settings.endGroup();
  };
  read("Hunter",hunter,NewCqZone|NewItuZone|NewDxcc|NewBandModeDxcc|NewGrid);
  read("Enthusiast",enthusiast,NewCall|NewBandModeCall);
}
void ManagedSettings::write(QSettings& settings) const {
  auto write=[&](QString name,ManagedProfile const& p) {
    settings.beginGroup("Managed/"+name);
    settings.setValue("Criteria",p.criteria);settings.setValue("AnswerWorked",p.answerWorked);
    settings.setValue("CallLimit",qBound(0,p.callLimit,99));settings.endGroup();
  };
  write("Hunter",hunter);write("Enthusiast",enthusiast);
}
void ManagedSession::setMode(ManagedMode mode,qint64 now) {mode_=mode;restartSchedule(now);}
void ManagedSession::restartSchedule(qint64 now) {clearCandidates();cqRemaining_=0;nextCq_=now+15*60000;}
void ManagedSession::cancelAttempt() {current_.clear();transmissions_=0;replied_=false;}
void ManagedSession::clearCandidates() {candidates_.clear();cancelAttempt();}
void ManagedSession::observe(ManagedCandidate const& candidate) {
  if(!active() || candidate.call.isEmpty())return;
  auto key=Radio::base_callsign(candidate.call);
  if(key==Radio::base_callsign(current_) && candidate.incoming && transmissions_>0)replied_=true;
  auto value=candidate;
  // A queued caller may subsequently send CQ. A message to someone else
  // cancels that pending invitation, rather than allowing an obsolete reply.
  if(value.grid.isEmpty())value.grid=candidates_.value(key).grid;
  candidates_.insert(key,value);
  if(candidates_.size()>1000) {
    for(auto it=candidates_.begin();it!=candidates_.end();)
      if(candidate.seen-it->seen>120000)it=candidates_.erase(it);else ++it;
  }
}
QList<ManagedCandidate> ManagedSession::candidates(qint64 now,qint64 outgoingAge) const {
  QList<ManagedCandidate> result;
  for(auto const& c:candidates_)
    if(now>=c.seen && now-c.seen<=(c.incoming?120000:outgoingAge))result.append(c);
  return result;
}
int ManagedSession::rank(unsigned novelty) {
  for(int i=0;i<7;++i)if(novelty&(1u<<i))return i;
  return 7;
}
bool ManagedSession::blocked(QString const& call,qint64 now) const {return retries_.value(Radio::base_callsign(call)).until>now;}
ManagedCandidate ManagedSession::choose(QList<ManagedCandidate> const& candidates,ManagedProfile const& p,qint64 now,bool incomingOnly) const {
  ManagedCandidate best;
  if(!active())return best;
  for(auto const& c:candidates) {
    auto key=Radio::base_callsign(c.call);
    if(c.call.isEmpty() || completed_.value(key,-1)>=c.seen)continue;
    // A station that now calls us back has responded: the outgoing retry ban
    // must not prevent that incoming QSO.
    if(c.incoming) {if(c.workedBandMode && !p.answerWorked)continue;}
    else if(incomingOnly || !c.available || !(c.novelty&p.criteria) || blocked(c.call,now))continue;
    if(best.call.isEmpty() || (c.incoming && !best.incoming) ||
       (c.incoming==best.incoming && (rank(c.novelty)<rank(best.novelty) ||
        (rank(c.novelty)==rank(best.novelty) && (c.snr>best.snr || (c.snr==best.snr && c.call<best.call))))))best=c;
  }
  return best;
}
void ManagedSession::begin(QString const& call) {current_=call;transmissions_=0;replied_=false;}
void ManagedSession::transmitted() {if(!current_.isEmpty())++transmissions_;}
bool ManagedSession::exhausted(int limit) const {return limit>0 && !replied_ && transmissions_>=limit;}
void ManagedSession::failed(qint64 now) {
  if(current_.isEmpty())return;
  auto key=Radio::base_callsign(current_);auto& r=retries_[key];
  if(mode_==ManagedMode::QsoEnthusiast) {
    ++r.failures;r.until=qMax(r.until,r.failures>=3?std::numeric_limits<qint64>::max():now+(r.failures==1?10:30)*60000);
  } else r.until=qMax(r.until,now+10*60000);
  candidates_.remove(key);cancelAttempt();
}
void ManagedSession::complete(qint64 now) {
  if(!current_.isEmpty())completed_.insert(Radio::base_callsign(current_),now);
  cancelAttempt();
}
bool ManagedSession::cqDue(qint64 now) {
  if(!active())return false;
  if(now>=nextCq_ && cqRemaining_==0)cqRemaining_=3;
  return cqRemaining_>0;
}
void ManagedSession::cqTransmitted(qint64 now) {
  if(cqRemaining_>0 && --cqRemaining_==0)nextCq_=now+15*60000;
}
