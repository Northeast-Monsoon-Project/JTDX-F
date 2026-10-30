#ifndef JTDXF_STATION_PROFILES_HPP
#define JTDXF_STATION_PROFILES_HPP
#include <QCoreApplication>
#include <QDialog>
#include <QJsonArray>
#include <QList>
#include <QString>
struct StationProfile {
  QString call, grid;
  int region=0, type2=1;
};
namespace StationProfiles {
QString normalize(QString call);
QString validate(StationProfile const& profile);
QString folder(QString call);
QList<StationProfile> decode(QByteArray const& json);
QByteArray encode(QList<StationProfile> const& profiles);
}
class QListWidget;
class StationManager : public QDialog {
  Q_DECLARE_TR_FUNCTIONS(StationManager)
public:
  explicit StationManager(QList<StationProfile> profiles, QWidget *parent=nullptr);
  QList<StationProfile> profiles() const { return profiles_; }
private:
  QList<StationProfile> profiles_;
  QListWidget *list_;
  void refresh(int selected=-1);
  void edit(int index);
};
#endif
