#include "StationProfiles.hpp"
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QSet>
#include <QVBoxLayout>
QString StationProfiles::normalize(QString call) { return call.trimmed().toUpper(); }
QString StationProfiles::folder(QString call) { return normalize(call).replace('/','_'); }
QString StationProfiles::validate(StationProfile const& p) {
  const auto call=normalize(p.call);
  static QRegularExpression calls("^(?=.{3,12}$)(?=.*[A-Z])(?=.*[0-9])[A-Z0-9]+(?:/[A-Z0-9]+){0,2}$");
  static QRegularExpression reserved("^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])$");
  static QRegularExpression grids("^[A-R]{2}[0-9]{2}(?:[A-X]{2}(?:[0-9]{2})?)?$");
  if (!calls.match(call).hasMatch() || reserved.match(folder(call)).hasMatch())
    return QCoreApplication::translate("StationManager","Enter a valid callsign (3–12 letters/digits, optional /suffix).");
  if (!grids.match(p.grid.trimmed().toUpper()).hasMatch())
    return QCoreApplication::translate("StationManager","Enter a valid 4, 6 or 8 character grid locator.");
  if (p.region<0 || p.region>3 || p.type2<0 || p.type2>2) return QStringLiteral("Invalid station options");
  return {};
}
QList<StationProfile> StationProfiles::decode(QByteArray const& json) {
  QList<StationProfile> out; QSet<QString> seen;
  for (auto v: QJsonDocument::fromJson(json).array()) {
    auto o=v.toObject(); StationProfile p;
    p.call=normalize(o.value("call").toString()); p.grid=o.value("grid").toString().trimmed().toUpper();
    p.region=o.value("region").toInt(); p.type2=o.value("type2").toInt(1);
    if (validate(p).isEmpty() && !seen.contains(p.call)) { out.append(p);seen.insert(p.call); }
  }
  return out;
}
QByteArray StationProfiles::encode(QList<StationProfile> const& profiles) {
  QJsonArray a; for(auto const& p: profiles) a.append(QJsonObject{{"call",normalize(p.call)},{"grid",p.grid.toUpper()},{"region",p.region},{"type2",p.type2}});
  return QJsonDocument(a).toJson(QJsonDocument::Compact);
}
StationManager::StationManager(QList<StationProfile> profiles,QWidget *parent):QDialog(parent),profiles_(profiles) {
  setObjectName("station_manager");setWindowTitle(tr("Callsign management"));resize(560,390);
  auto layout=new QVBoxLayout(this);
  list_=new QListWidget(this);list_->setObjectName("station_list");layout->addWidget(list_);
  auto row=new QHBoxLayout;layout->addLayout(row);
  for(auto item: {qMakePair(tr("Add"),0),qMakePair(tr("Edit"),1),qMakePair(tr("Remove profile"),2)}) {
    auto button=new QPushButton(item.first,this);row->addWidget(button);
    button->setObjectName(QString("station_action_%1").arg(item.second));
    connect(button,&QPushButton::clicked,this,[this,item] {
      int n=list_->currentRow();
      if(item.second==0) edit(-1);
      else if(n>=0 && item.second==1) edit(n);
      else if(n>=0) { profiles_.removeAt(n);refresh(qMin(n,profiles_.size()-1)); }
    });
  }
  connect(list_,&QListWidget::itemDoubleClicked,this,[this]{edit(list_->currentRow());});
  auto buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,this);layout->addWidget(buttons);
  buttons->button(QDialogButtonBox::Ok)->setText(tr("OK"));
  buttons->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
  connect(buttons,&QDialogButtonBox::accepted,this,&QDialog::accept);
  connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);refresh(0);
}
void StationManager::refresh(int selected) {
  list_->clear();for(auto const& p: profiles_) list_->addItem(p.call+"   "+p.grid);
  list_->setCurrentRow(selected);
}
void StationManager::edit(int index) {
  StationProfile p;if(index>=0) p=profiles_.at(index);
  QDialog dialog(this);dialog.setObjectName("station_editor");dialog.setWindowTitle(index<0?tr("Add callsign"):tr("Edit callsign"));
  auto form=new QFormLayout(&dialog);
  auto call=new QLineEdit(p.call,&dialog);call->setObjectName("profile_call");call->setMaxLength(12);
  auto grid=new QLineEdit(p.grid,&dialog);grid->setObjectName("profile_grid");grid->setMaxLength(8);
  auto region=new QComboBox(&dialog);region->setObjectName("profile_region");region->addItems({tr("All regions"),tr("Region 1"),tr("Region 2"),tr("Region 3")});region->setCurrentIndex(p.region);
  auto type2=new QComboBox(&dialog);type2->setObjectName("profile_type2");type2->addItems({tr("Full call in Tx1"),tr("Full call in Tx3"),tr("Full call in Tx5 only")});type2->setCurrentIndex(p.type2);
  form->addRow(tr("My callsign:"),call);form->addRow(tr("My grid:"),grid);form->addRow(tr("IARU region:"),region);form->addRow(tr("Type 2 callsign messages:"),type2);
  auto buttons=new QDialogButtonBox(QDialogButtonBox::Save|QDialogButtonBox::Cancel,&dialog);form->addRow(buttons);
  buttons->button(QDialogButtonBox::Save)->setText(tr("Save"));
  buttons->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
  connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
  connect(buttons,&QDialogButtonBox::accepted,&dialog,[&] {
    p.call=StationProfiles::normalize(call->text());p.grid=grid->text().trimmed().toUpper();p.region=region->currentIndex();p.type2=type2->currentIndex();
    QString error=StationProfiles::validate(p);
    for(int n=0;n<profiles_.size();++n) if(n!=index && profiles_[n].call==p.call) error=tr("This callsign already exists. Different /suffixes are allowed.");
    if(!error.isEmpty()) { QMessageBox::warning(&dialog,tr("Invalid profile"),error);return; }
    dialog.accept();
  });
  if(dialog.exec()==QDialog::Accepted) { if(index<0) {index=profiles_.size();profiles_.append(p);} else profiles_[index]=p;refresh(index); }
}
