#include "about.h"

#include <QCoreApplication>
#include <QString>
#include <QTextBrowser>
#include <QScreen>
#include <QGuiApplication>

#include "revision_utils.hpp"

#include "ui_about.h"
#include "moc_about.cpp"

CAboutDlg::CAboutDlg(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::CAboutDlg)
{
  ui->setupUi(this);
  auto text = new QTextBrowser(this);
  ui->horizontalLayout_2->removeWidget(ui->labelTxt);
  ui->labelTxt->hide();
  text->setFrameShape(QFrame::NoFrame);
  text->setOpenExternalLinks(true);
  ui->horizontalLayout_2->addWidget(text);
  ui->verticalSpacer->changeSize(0, 0, QSizePolicy::Minimum, QSizePolicy::Fixed);
  setWindowTitle(tr("About JTDX-F"));

  text->setHtml ("<html><div align='center'><img src=':/logo-full.png' width='210' height='210'/><h2>"
                         + QString("JTDX-F V")
                         + QCoreApplication::applicationVersion () + "  HF software" + "</h2>\n\n"
                         + tr("JTDX-F is developed by the Fluxmancer team.<br>Based on JTDX and WSJT-X; upstream credits and GPLv3 are preserved.<br><br>") +
                         "It is modified WSJT-X software forked from WSJT-X v1.7 r6462, <br>"
                         "FT8 code is forked from WSJT-X v1.8 and v2.0, <br>"
                         "FT4 code is forked from WSJT-X v2.1, <br>"
                         "WSPR code is forked from WSJT-X v2.1.2, <br>"
                         "&copy; 2001-2022 by Joe Taylor, K1JT, Bill Somerville, G4WJS, <br>"
                         "Steve Franke, K9AN, and Nico Palermo, IV3NWV.<br><br>"
                         "JTDX supports FT8, FT4, JT9, T10 and JT65A digital modes for HF <br>"
                         "amateur radio communication, focused on DXing <br>"
                         "and being shaped by community of DXers. <br>"
                         "&copy; 2016-2022 by Igor Chernikov, UA3DJY, and Arvo Järve, ES1JA. <br>"
                         "It is created with contributions from 5P1KZX, 9A5CW, BD3OOX, CE2EC, CT1AXS,<br>"
                         "DK7UY, DO1IP, EA1AHY, EA3W, EA7QL, ES2HV, ES2MC, ES4AW, ES5TF, F1DSZ, F5RUE,<br>"
                         "G4UJS, G7OED, HA3LI, IZ5ILJ, JA2BQX, JG1APX, JP1LRT, LU9DO, MM0HVU, N6ML,<br>"
                         "NL9222, OE1MWW, ON3CQ, PA7TWO, PP5FMM, R3BB, RK3AOL, RX3ASP, RA4UDC, RW4O, R0JF,<br>"
                         "SM0LTV, SP2L, SV1IYF, UA3ALE, US-E-12, VE3NEA, VK3AMA, VK6KXW, VK7YUM, VR2UPU,<br>"
                         "W9MDB, YL3GBC family and LY3BG family: Vytas and Rimas Kudelis. <br><br>"
                         + QString("SuperFox receive: WSJT-X, Nico Palermo IV3NWV and CE3TSK (JTDX_CONTEST).<br><br>")
                         + tr("JTDX-F is licensed under GNU GPLv3, without warranty.") + "<br>"
                         "<a href=\"https://www.gnu.org/licenses/gpl-3.0.txt\">"
                         "https://www.gnu.org/licenses/gpl-3.0.txt</a></div></html>");
  auto area = QGuiApplication::primaryScreen()->availableGeometry();
  resize(qMin(780, area.width()-40), qMin(690, area.height()-60));
}

CAboutDlg::~CAboutDlg()
{
}
