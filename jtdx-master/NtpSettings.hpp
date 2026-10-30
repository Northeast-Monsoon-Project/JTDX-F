#ifndef JTDXF_NTP_SETTINGS_HPP
#define JTDXF_NTP_SETTINGS_HPP
#include <QGroupBox>
#include "NtpInterval.hpp"
class QComboBox;class QLineEdit;class QLabel;
struct NtpSettings { bool enabled=false; QString server="ntp1.aliyun.com"; bool custom=false; int intervalMinutes=15; };
struct NtpPreset { QString label,host,continent,country,provider; };
class NtpSettingsWidget : public QGroupBox {
  Q_OBJECT
public:
  explicit NtpSettingsWidget(QWidget *parent=nullptr);
  static QList<NtpPreset> presets();
  static QList<int> intervals() {return ntpIntervals();}
  NtpSettings settings() const;
  void setSettings(NtpSettings const& settings);
  void setStatus(QString const& message);
private:
  QComboBox *servers_,*interval_;QLineEdit *custom_;QLabel *status_;
};
#endif
