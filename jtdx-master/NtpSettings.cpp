#include "NtpSettings.hpp"
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSignalBlocker>
QList<NtpPreset> NtpSettingsWidget::presets() {
  QList<NtpPreset> list;
  auto add=[&](QString label,QString host,QString continent,QString country,QString provider){list.append({label+" — "+host,host,continent,country,provider});};
  add(tr("Asia / China / Alibaba") + " 1","ntp1.aliyun.com","Asia","CN","Alibaba");
  add(tr("Asia / China / Alibaba") + " 2","ntp2.aliyun.com","Asia","CN","Alibaba");
  add(tr("Asia / China / Tencent") + " 1","ntp.tencent.com","Asia","CN","Tencent");
  add(tr("Asia / China / Tencent") + " 2","ntp1.tencent.com","Asia","CN","Tencent");
  add(tr("Asia / China / CAS NTSC"),"ntp.ntsc.ac.cn","Asia","CN","NTSC");
  add(tr("Asia / China / Baidu"),"ntp1.baidu.com","Asia","CN","Baidu");
  add(tr("Asia / China / NTP Pool"),"cn.pool.ntp.org","Asia","CN","NTP Pool");
  add(tr("Asia / China / Tsinghua TUNA"),"ntp.tuna.tsinghua.edu.cn","Asia","CN","TUNA");
  add(tr("Asia / Japan / NICT"),"ntp.nict.jp","Asia","JP","NICT");
  add(tr("Asia / Japan / Internet Multifeed") + " 1","ntp1.jst.mfeed.ad.jp","Asia","JP","MFEED");
  add(tr("Asia / Japan / Internet Multifeed") + " 2","ntp2.jst.mfeed.ad.jp","Asia","JP","MFEED");
  add(tr("Asia / Japan / Sakura Internet"),"ntp1.sakura.ad.jp","Asia","JP","Sakura");
  add(tr("Asia / South Korea / KRISS"),"ntp.kriss.re.kr","Asia","KR","KRISS");
  add(tr("Asia / Apple"),"time.asia.apple.com","Asia","","Apple");
  add(tr("Europe / Germany / PTB") + " 1","ptbtime1.ptb.de","Europe","DE","PTB");
  add(tr("Europe / Germany / PTB") + " 2","ptbtime2.ptb.de","Europe","DE","PTB");
  add(tr("Europe / UK / NPL") + " 1","ntp1.npl.co.uk","Europe","GB","NPL");
  add(tr("Europe / UK / NPL") + " 2","ntp2.npl.co.uk","Europe","GB","NPL");
  add(tr("Europe / Sweden / Netnod") + " 1","gbg1.ntp.netnod.se","Europe","SE","Netnod");
  add(tr("Europe / Sweden / Netnod") + " 2","gbg2.ntp.netnod.se","Europe","SE","Netnod");
  add(tr("Europe / Poland / GUM") + " 1","tempus1.gum.gov.pl","Europe","PL","GUM");
  add(tr("Europe / Poland / GUM") + " 2","tempus2.gum.gov.pl","Europe","PL","GUM");
  add(tr("Europe / France / RDEM Systems") + " 1","ntp-1.rdem-systems.com","Europe","FR","RDEM");
  add(tr("Europe / France / RDEM Systems") + " 2","ntp-2.rdem-systems.com","Europe","FR","RDEM");
  add(tr("Africa / South Africa / SANReN") + " 1","tick.sanren.ac.za","Africa","ZA","SANReN");
  add(tr("Africa / South Africa / SANReN") + " 2","tock.sanren.ac.za","Africa","ZA","SANReN");
  add(tr("Africa / South Africa / INX-ZA") + " 1","ntp1.inx.net.za","Africa","ZA","INX-ZA");
  add(tr("Africa / South Africa / INX-ZA") + " 2","ntp2.inx.net.za","Africa","ZA","INX-ZA");
  add(tr("Africa / South Africa / NMISA"),"time.nmisa.org","Africa","ZA","NMISA");
  add(tr("Africa / Kenya / icolo") + " 1","ntp0.icolo.io","Africa","KE","icolo");
  add(tr("Africa / Kenya / icolo") + " 2","ntp1.icolo.io","Africa","KE","icolo");
  add(tr("Africa / Mauritius / Mauritius Telecom"),"ntp.telecom.mu","Africa","MU","Mauritius Telecom");
  add(tr("North America / USA / NIST"),"time.nist.gov","North America","US","NIST");
  add(tr("North America / Canada / NRC") + " 1","time.nrc.ca","North America","CA","NRC");
  add(tr("North America / Canada / NRC") + " 2","time.chu.nrc.ca","North America","CA","NRC");
  add(tr("South America / Brazil / NIC.br") + " 1","a.ntp.br","South America","BR","NIC.br");
  add(tr("South America / Brazil / NIC.br") + " 2","b.ntp.br","South America","BR","NIC.br");
  add(tr("South America / Brazil / National Observatory") + " 1","200.20.186.75","South America","BR","Observatorio Nacional");
  add(tr("South America / Brazil / National Observatory") + " 2","200.20.186.94","South America","BR","Observatorio Nacional");
  add(tr("South America / Chile / SHOA"),"ntp.shoa.cl","South America","CL","SHOA");
  add(tr("South America / Colombia / INM") + " 1","ntp1.inm.gov.co","South America","CO","INM");
  add(tr("South America / Colombia / INM") + " 2","ntp2.inm.gov.co","South America","CO","INM");
  add(tr("South America / Argentina / INTI"),"ntp.inti.gob.ar","South America","AR","INTI");
  add(tr("Oceania / Australia / NTP Pool"),"au.pool.ntp.org","Oceania","AU","NTP Pool");
  add(tr("Oceania / New Zealand / MSL (NZ networks only)"),"pool.msltime.measurement.govt.nz","Oceania","NZ","MSL");
  add(tr("Global / Microsoft Windows"),"time.windows.com","Global","","Microsoft");
  add(tr("Global / Apple"),"time.apple.com","Global","","Apple");
  add(tr("Global / Google") + " 1","time1.google.com","Global","","Google");
  add(tr("Global / Google") + " 2","time2.google.com","Global","","Google");
  add(tr("Global / Cloudflare"),"time.cloudflare.com","Global","","Cloudflare");
  return list;
}
NtpSettingsWidget::NtpSettingsWidget(QWidget *parent):QGroupBox(tr("NTP time synchronization (requests administrator privileges when enabled)"),parent) {
  setObjectName("ntp_group");setCheckable(true);setChecked(false);
  auto layout=new QVBoxLayout(this);
  servers_=new QComboBox(this);servers_->setObjectName("ntp_server");servers_->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);servers_->setMinimumContentsLength(27);
  for(auto const& p:presets())servers_->addItem(p.label,p.host);
  servers_->addItem(tr("Other"),QString());
  custom_=new QLineEdit(this);custom_->setObjectName("ntp_custom");custom_->setPlaceholderText(tr("NTP hostname or IP address"));custom_->setMaxLength(253);
  status_=new QLabel(tr("Uses system UTC time; the synchronization interval is configurable."),this);status_->setObjectName("ntp_status");status_->setWordWrap(true);
  auto serverRow=new QHBoxLayout;
  serverRow->addWidget(servers_,1);serverRow->addWidget(custom_,1);
  layout->addLayout(serverRow);
  auto intervalRow=new QHBoxLayout;
  interval_=new QComboBox(this);interval_->setObjectName("ntp_interval");
  for(int minutes:intervals())interval_->addItem(minutes<60?tr("%1 minutes").arg(minutes):tr("%1 hours").arg(minutes/60),minutes);
  interval_->setCurrentIndex(interval_->findData(15));
  auto intervalLabel=new QLabel(tr("Synchronization interval:"),this);intervalLabel->setBuddy(interval_);
  intervalRow->addWidget(intervalLabel);intervalRow->addWidget(interval_);intervalRow->addStretch();
  layout->addLayout(intervalRow);layout->addWidget(status_);
  servers_->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
  connect(servers_,QOverload<int>::of(&QComboBox::currentIndexChanged),this,[this]{
    bool other=servers_->currentData().toString().isEmpty();
    custom_->setVisible(other);
  });
  custom_->hide();
}
NtpSettings NtpSettingsWidget::settings() const {
  NtpSettings s;s.enabled=isChecked();s.custom=servers_->currentData().toString().isEmpty();s.server=s.custom?custom_->text().trimmed():servers_->currentData().toString();s.intervalMinutes=interval_->currentData().toInt();return s;
}
void NtpSettingsWidget::setSettings(NtpSettings const& s) {
  QSignalBlocker blocked(this);setChecked(s.enabled);
  interval_->setCurrentIndex(interval_->findData(ntpIntervalMinutes(s.intervalMinutes)));
  int n=s.custom?-1:servers_->findData(s.server);servers_->setCurrentIndex(n<0?servers_->count()-1:n);custom_->setText(n<0?s.server:QString());
  custom_->setVisible(n<0);
}
void NtpSettingsWidget::setStatus(QString const& message){status_->setText(message);}
