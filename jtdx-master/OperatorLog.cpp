#include "OperatorLog.hpp"
#include "StationProfiles.hpp"
#include "logbook/adif.h"
#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QLockFile>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSet>
#include <QStandardPaths>
#include <QTextStream>
#include <algorithm>
namespace {
QString header() { return "JTDX-F quarterly operator log\n<ADIF_VER:5>3.1.4 <PROGRAMID:6>JTDX-F <EOH>\n"; }
bool save(QString const& path,QByteArray const& bytes,QString *error) {
  QSaveFile f(path);
  if(!f.open(QIODevice::WriteOnly) || f.write(bytes)!=bytes.size() || !f.commit()) {
    if(error) *error=f.errorString();
    return false;
  }
  return true;
}
QString keyField(OperatorLog::Record const& r,QString key) { return r.value(key).trimmed().toUpper(); }
}
QDir OperatorLog::root() { return QDir(QStandardPaths::writableLocation(QStandardPaths::DataLocation)); }
QDir OperatorLog::directory(QString const& call,QDir base) {
  QString name=StationProfiles::folder(call);
  if(name.isEmpty() || name.contains(QRegularExpression("[^A-Z0-9_]"))) return base;
  base.mkpath(name);return QDir(base.filePath(name));
}
QString OperatorLog::quarterFile(QString const& call,QDate date,QDir base) {
  if(call.isEmpty() || !date.isValid()) return {};
  return directory(call,base).filePath(StationProfiles::folder(call)+QString::number(date.year())+"S"+QString::number((date.month()-1)/3+1)+".adi");
}
bool OperatorLog::isQuarterFile(QString const& path) {
  return QRegularExpression("^[A-Z0-9_]+[0-9]{4}S[1-4]\\.adi$",QRegularExpression::CaseInsensitiveOption).match(QFileInfo(path).fileName()).hasMatch();
}
QStringList OperatorLog::files(QString const& call,QDir base) {
  if(call.isEmpty()) return {};
  auto dir=directory(call,base);QStringList out;
  for(auto const& f:dir.entryInfoList(QDir::Files,QDir::Name))
    if(f.suffix().compare("adi",Qt::CaseInsensitive)==0 || f.suffix().compare("adif",Qt::CaseInsensitive)==0) out.append(f.absoluteFilePath());
  return out;
}
QList<OperatorLog::Record> OperatorLog::parse(QString const& text,QString *error) {
  QList<Record> out;Record record;int pos=0;
  if(error) error->clear();
  while(pos<text.size()) {
    int start=text.indexOf('<',pos);if(start<0) break;
    int end=text.indexOf('>',start+1);
    if(end<0) { if(error) *error="Incomplete ADIF field header";return {}; }
    auto parts=text.mid(start+1,end-start-1).split(':');QString name=parts[0].trimmed().toUpper();
    pos=end+1;
    if(name=="EOH") {record.clear();continue;}
    if(name=="EOR") {if(!record.isEmpty()) out.append(record);record.clear();continue;}
    if(parts.size()<2 || parts.size()>3 || !QRegularExpression("^[A-Z0-9_]+$").match(name).hasMatch()) {
      if(error) *error="Invalid ADIF field: "+name;
      return {};
    }
    bool ok=false;int n=parts[1].toInt(&ok);
    if(!ok || n<0 || n>text.size()-pos) {if(error) *error="Truncated ADIF value: "+name;return {};}
    record.insert(name,text.mid(pos,n));pos+=n;
  }
  if(!record.isEmpty()) {if(error) *error="ADIF record is missing <EOR>";return {};}
  return out;
}
QList<OperatorLog::Record> OperatorLog::read(QString const& path,QString *error) {
  if(error) error->clear();
  QFile f(path);if(!f.open(QIODevice::ReadOnly)) {if(error) *error=f.errorString();return {};}
  QTextStream in(&f);in.setCodec("UTF-8");auto text=in.readAll();
  if(QFileInfo(path).suffix().compare("adi",Qt::CaseInsensitive)==0 || QFileInfo(path).suffix().compare("adif",Qt::CaseInsensitive)==0 || text.contains(QRegularExpression("<EOR>",QRegularExpression::CaseInsensitiveOption))) return parse(text,error);
  // Older JTDX also wrote a comma-separated wsjtx.log companion file.
  if(QFileInfo(path).suffix().compare("log",Qt::CaseInsensitive)==0) {
    QList<Record> records;
    for(auto line:text.split('\n')) {
      if(line.trimmed().isEmpty()) continue;
      auto fields=line.trimmed().split(',');
      if(fields.size()<9) {if(error) *error="Unsupported or incomplete legacy log row";return {};}
      auto date=QDate::fromString(fields[0],"yyyy-MM-dd");
      if(!date.isValid()) {if(error) *error="Invalid legacy log date";return {};}
      Record r{{"QSO_DATE",date.toString("yyyyMMdd")},{"TIME_ON",QString(fields[1]).remove(':')},{"CALL",fields[4].toUpper()},
               {"GRIDSQUARE",fields[5].toUpper()},{"FREQ",fields[6]},{"MODE",fields[7].toUpper()},{"RST_SENT",fields[8]}};
      if(fields.size()>9) r.insert("RST_RCVD",fields[9]);
      if(fields.size()>10) r.insert("TX_PWR",fields[10]);
      if(fields.size()>12) {
        r.insert("COMMENT",fields.mid(11,fields.size()-12).join(','));
        r.insert("NAME",fields.last());
      } else if(fields.size()>11) r.insert("COMMENT",fields[11]);
      auto offDate=QDate::fromString(fields[2],"yyyy-MM-dd");
      if(offDate.isValid()) r.insert("QSO_DATE_OFF",offDate.toString("yyyyMMdd"));
      r.insert("TIME_OFF",QString(fields[3]).remove(':'));
      records.append(r);
    }
    return records;
  }
  if(error) *error="No complete ADIF records found";
  return {};
}
QString OperatorLog::serialize(Record const& record) {
  QString out;for(auto i=record.begin();i!=record.end();++i) out+='<'+i.key()+':'+QString::number(i.value().size())+'>'+i.value();
  return out+"<EOR>\n";
}
QString OperatorLog::identity(Record const& r,QString const& fallback) {
  QString mode=keyField(r,"MODE");if(mode=="MFSK") mode=keyField(r,"SUBMODE");
  QString station=keyField(r,"STATION_CALLSIGN");if(station.isEmpty()) station=StationProfiles::normalize(fallback);
  QString time=keyField(r,"TIME_ON").leftJustified(6,'0');
  QString band=keyField(r,"BAND");if(band.isEmpty()) band=ADIF::bandFromFrequency(r.value("FREQ").toDouble()).toUpper();
  return station+'|'+keyField(r,"CALL")+'|'+keyField(r,"QSO_DATE")+'|'+time+'|'+mode+'|'+band;
}
bool OperatorLog::append(QString const& path,QString const& adif,QString *error) {
  if(path.isEmpty()) {if(error) *error="Select a callsign before logging";return false;}
  QDir dir=QFileInfo(path).dir();if(!dir.mkpath(".")) {if(error) *error="Cannot create log directory";return false;}
  QLockFile lock(dir.filePath(".operator-log.lock"));
  if(!lock.tryLock(3000)) {if(error) *error="Log is busy; retry after import finishes";return false;}
  QFile f(path);bool fresh=!f.exists() || f.size()==0;
  if(!f.open(QIODevice::WriteOnly|QIODevice::Append)) {if(error) *error=f.errorString();return false;}
  qint64 originalSize=f.size();
  QByteArray bytes=((fresh?header():QString())+adif).toUtf8();
  if(f.write(bytes)!=bytes.size() || !f.flush()) {
    if(error) *error=f.errorString();
    f.resize(originalSize); f.flush();
    return false;
  }
  return true;
}
OperatorLog::ImportResult OperatorLog::importFiles(QString active,QStringList paths,QDir base) {
  ImportResult result;active=StationProfiles::normalize(active);
  if(active.isEmpty()) return result;
  paths.removeDuplicates();
  for(auto const& path:paths) {
    QString error;auto records=read(path,&error);
    if(!error.isEmpty() || records.isEmpty()) {result.errors.append(path+": "+(error.isEmpty()?"Empty log":error));continue;}
    QMap<QString,QList<Record>> groups;bool valid=true;
    for(auto record:records) {
      QString call=StationProfiles::normalize(record.value("STATION_CALLSIGN"));if(call.isEmpty()) call=active;
      StationProfile p;p.call=call;p.grid="AA00";
      QDate date=QDate::fromString(record.value("QSO_DATE"),"yyyyMMdd");
      if(!StationProfiles::validate(p).isEmpty() || !date.isValid() || record.value("CALL").trimmed().isEmpty()) {
        result.errors.append(path+": invalid station, QSO date or contact callsign; original retained");valid=false;break;
      }
      record["STATION_CALLSIGN"]=call;
      groups[quarterFile(call,date,base)].append(record);
    }
    if(!valid) continue;
    // Each quarter is an atomic replacement. Deduplication makes a retry after
    // a partial multi-quarter import idempotent. Archive only after all commits.
    for(auto group=groups.begin();group!=groups.end();++group) {
      QDir dir=QFileInfo(group.key()).dir();QLockFile lock(dir.filePath(".operator-log.lock"));
      if(!lock.tryLock(10000)) {error="Log is busy";break;}
      QList<Record> existing;if(QFile::exists(group.key())) existing=read(group.key(),&error);
      if(!error.isEmpty()) break;
      QSet<QString> seen;QString body=header();
      for(auto const& r:existing) {seen.insert(identity(r));body+=serialize(r);}
      int added=0;
      for(auto const& r:group.value()) {
        auto id=identity(r);if(seen.contains(id)) {++result.duplicates;continue;}
        seen.insert(id);body+=serialize(r);++added;
      }
      if(added && !save(group.key(),body.toUtf8(),&error)) break;
      result.imported+=added;
    }
    if(!error.isEmpty()) {result.errors.append(path+": "+error+"; original retained");continue;}
    QFile original(path);if(!original.open(QIODevice::ReadOnly)) {result.errors.append(path+": cannot archive original");continue;}
    auto bytes=original.readAll();original.close();
    auto archive=directory(active,base);
    if(!archive.mkpath("old") || !archive.cd("old")) {
      result.errors.append(path+": cannot create old backup directory; original retained");
      continue;
    }
    QString name=QFileInfo(path).fileName();QString dest=archive.filePath(name);
    if(QFile::exists(dest)) {
      QFile prior(dest);prior.open(QIODevice::ReadOnly);
      if(prior.readAll()!=bytes) dest=archive.filePath(QFileInfo(name).completeBaseName()+"-"+QString::fromLatin1(QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex().left(12))+'.'+QFileInfo(name).suffix());
    }
    if(!QFile::exists(dest) && !save(dest,bytes,&error)) {result.errors.append(path+": "+error);continue;}
    // External drag sources are copied, never removed. Files deposited into the
    // operator directory are moved logically into old after the backup commits.
    if(QFileInfo(path).absolutePath()==directory(active,base).absolutePath() && !isQuarterFile(path)) {
      if(!QFile::remove(path)) {result.errors.append(path+": converted, but original could not be moved to old");continue;}
    }
    ++result.archived;
  }
  return result;
}
