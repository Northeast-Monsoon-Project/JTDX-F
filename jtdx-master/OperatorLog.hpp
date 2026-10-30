#ifndef JTDXF_OPERATOR_LOG_HPP
#define JTDXF_OPERATOR_LOG_HPP
#include <QDate>
#include <QDir>
#include <QMap>
#include <QStringList>
#include <QList>
namespace OperatorLog {
using Record=QMap<QString,QString>;
struct ImportResult { int imported=0, duplicates=0, archived=0; QStringList errors; };
QDir root();
QDir directory(QString const& callsign, QDir base=root());
QString quarterFile(QString const& callsign,QDate utcDate,QDir base=root());
QStringList files(QString const& callsign,QDir base=root());
QList<Record> parse(QString const& text,QString *error=nullptr);
QList<Record> read(QString const& path,QString *error=nullptr);
QString serialize(Record const& record);
QString identity(Record const& record,QString const& fallbackCall={});
bool isQuarterFile(QString const& path);
ImportResult importFiles(QString callsign,QStringList paths,QDir base=root());
bool append(QString const& path,QString const& adif,QString *error=nullptr);
}
#endif
