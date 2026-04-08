/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDir>
#include <QSettings>
#include <QResizeEvent>
#include <QScreen>
#include <QShowEvent>
#include <QStyleHints>
#include <QScrollBar>
#include <QMessageBox>
#include "keyenterreceiver.h"
#include "../clientv2.h"

using namespace std::chrono_literals;

extern std::unique_ptr<QSettings> settings;

MainWindow::MainWindow(QWidget *parent, int argc, char ** argv)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    updateColorScheme(QApplication::styleHints()->colorScheme());
    QObject::connect(QApplication::styleHints(),
                     &QStyleHints::colorSchemeChanged,
                     this, &MainWindow::updateColorScheme);

    auto menubarFont = ui->menubar->font();
    menubarFont.setPointSize(12);
    ui->menubar->setFont(menubarFont);

    /* this is done instead of in *.ui for it does not cascade */
    ui->MainArea->setObjectName("mainArea");
    ui->MainArea->setStyleSheet("QWidget#mainArea { border-style: none }");
    lay = new QStackedLayout();

    /* Window centering moved to showEvent() because screen() and rect()
     * may not be accurate during construction on Linux */

    ui->ResourcesBar->hide();
    ui->OilLabel->setPixmap(QPixmap(QStringLiteral(
        ":/resources/resord/oil.png")));
    ui->ExploLabel->setPixmap(QPixmap(QStringLiteral(
        ":/resources/resord/explosive.png")));
    ui->SteelLabel->setPixmap(QPixmap(QStringLiteral(
        ":/resources/resord/steel.png")));
    ui->RubberLabel->setPixmap(QPixmap(QStringLiteral(
        ":/resources/resord/rubber.png")));
    ui->AluminumLabel->setPixmap(QPixmap(QStringLiteral(
        ":/resources/resord/aluminum.png")));
    ui->TungstenLabel->setPixmap(QPixmap(QStringLiteral(
        ":/resources/resord/tungsten.png")));
    ui->ChromiumLabel->setPixmap(QPixmap(QStringLiteral(
        ":/resources/resord/chromium.png")));
    ui->OilCount->setText(QStringLiteral("0"));
    ui->ExploCount->setText(QStringLiteral("0"));
    ui->SteelCount->setText(QStringLiteral("0"));
    ui->RubberCount->setText(QStringLiteral("0"));
    ui->AluminumCount->setText(QStringLiteral("0"));
    ui->TungstenCount->setText(QStringLiteral("0"));
    ui->ChromiumCount->setText(QStringLiteral("0"));

    KeyEnterReceiver *key = new KeyEnterReceiver(this);
    ui->CommandPrompt->installEventFilter(key);

    Client &engine = Client::getInstance();

    unlockBattle();
    connect(&engine, &Client::lockBattle,
            this, &MainWindow::lockBattle);
    connect(&engine, &Client::unlockBattle,
            this, &MainWindow::unlockBattle);
    connect(this, &MainWindow::cmdMessage,
            &engine, &Client::parse);
    connect(&engine, &Client::qout,
            this, &MainWindow::printMessage);
    connect(&engine, &Client::aboutToQuit,
            this, &MainWindow::close);
    connect(key, &KeyEnterReceiver::enterPressed,
            this, &MainWindow::processCmd);
    connect(&engine, &Client::gamestateChanged,
            this, &MainWindow::gamestateChanged);
    connect(ui->actionLogout, &QAction::triggered,
            &engine, &Client::parseDisconnectReq);
    connect(ui->actionExit, &QAction::triggered,
            &engine, &Client::parseQuit);
    connect(&engine, &Client::receivedResourceInfo,
            this, &MainWindow::updateResources);
    connect(ui->actionSettings, &QAction::triggered,
            &settingsWindow, &QDialog::show);
    connect(ui->actionLicense, &QAction::triggered,
            this, [this](){
                QMessageBox box(this);
                QString notice;
                QDir currentDir = QDir::current();
                QString openingwords = settings->value(
                    "license_notice", ":/openingwords.txt").toString();
                QFile licenseFile(currentDir.filePath(openingwords));
                if(Q_UNLIKELY(!licenseFile.open(
                        QIODevice::ReadOnly | QIODevice::Text))) {
                    //% "Can't find license file, exiting."
                    qFatal() << qtTrId("licence-not-found").toUtf8();
                }
                else {
                    QTextStream instream1(&licenseFile);
                    notice = instream1.readAll();
                }
                notice.replace("<https://www.gnu.org/licenses/>",
                               "<a href=\"https://www.gnu.org/licenses\">"
                               "the GNU.org page</a>");
                notice.replace("\n", "<br>");
                notice = "<p align='center'>" + notice + "</p>";
                box.setText(notice);
                box.exec();
            });
    connect(ui->actionAbout_Qt, &QAction::triggered,
            QApplication::instance(), &QApplication::aboutQt);
    connect(ui->actionBuyOrdResources, &QAction::triggered,
            this, &MainWindow::buyOrdResources);
    connect(ui->actionBuyARD, &QAction::triggered,
            this, &MainWindow::buyARD);
    connect(ui->actionBuyEquip, &QAction::triggered,
            this, &MainWindow::buyEquip);
    connect(ui->actionBuyMedal, &QAction::triggered,
            this, &MainWindow::buyMedal);

    portArea = new PortArea(this);
    licenseArea = new LicenseArea(this);
    newLoginScreen = new NewLoginS(this);
    factoryArea = new FactoryArea(this);
    techArea = new TechView(this);
    battleArea = new Sortie(this);
    fleetArea = new FleetView(this);
    repairArea = new Repair(this);

    lay->addWidget(portArea);
    lay->addWidget(licenseArea);
    lay->addWidget(newLoginScreen);
    lay->addWidget(factoryArea);
    lay->addWidget(techArea);
    lay->addWidget(battleArea);
    lay->addWidget(fleetArea);
    lay->addWidget(repairArea);

    ui->MainArea->setLayout(lay);
    lay->setContentsMargins(0,0,0,0);

    QTimer::singleShot(100ms, this,
                       [this]
                       {
                           lay->setCurrentWidget(licenseArea);
                           adjustArea(licenseArea,
                                      ui->MainArea->frameSize());
                       });
    connect(licenseArea, &LicenseArea::showLicenseComplete,
            lay, [this](){
                if(lay->currentWidget() == licenseArea) {
                    lay->setCurrentWidget(newLoginScreen);
                }
            });
    connect(licenseArea, &LicenseArea::showLicenseComplete,
            this, &MainWindow::gamestateInit);
    QTimer::singleShot(std::chrono::milliseconds(
                           settings->value("client/licenseareapersist",
                                           5000).toInt()), this,
                       [this]{
                           this->licenseArea->complete();
                       });
    connect(&engine, &Client::equipRegistryComplete,
            portArea, &PortArea::equipRegistryComplete);
    connect(&engine, &Client::shipRegistryComplete,
            portArea, &PortArea::shipRegistryComplete);
    connect(&engine, &Client::mapRegistryComplete,
            portArea, &PortArea::mapRegistryComplete);
    connect(&engine, &Client::tsunkitAssetsComplete,
            lay, [this](){
                lay->setCurrentWidget(portArea);
            });
    connect(&engine, &Client::tsunkitAssetsComplete,
            portArea, &PortArea::hello);

    connect(lay, &QStackedLayout::currentChanged,
            this, &MainWindow::adjust);

    connect(&engine, &Client::uiRefreshSig,
            this, qOverload<>(&MainWindow::update));
}

MainWindow::~MainWindow()
{
    delete ui;
}

FleetView * MainWindow::getFleetArea() const {
    return fleetArea;
}

QLayout * MainWindow::getFleetAreaWidget() const {
    return lay;
}

void MainWindow::lockBattle() {
    for(const auto &c: v) {
        disconnect(c);
    }
}

void MainWindow::unlockBattle() {
    Client &engine = Client::getInstance();
    v =
        {
            connect(ui->actionBack_to_naval_base, &QAction::triggered,
                    &engine, &Client::backToNavalBase),
            connect(ui->actionView_Tech, &QAction::triggered,
                    &engine, &Client::switchToTech),
            connect(ui->actionRepair, &QAction::triggered,
                    &engine, &Client::switchToRepairView),
            connect(ui->actionDevelop_Equipment, &QAction::triggered,
                    &engine, &Client::switchToFactory),
            connect(ui->actionDevelop_Equipment, &QAction::triggered,
                    this, &MainWindow::switchToDevelop),
            connect(ui->actionConstruct_Ships, &QAction::triggered,
                    &engine, &Client::switchToFactory),
            connect(ui->actionConstruct_Ships, &QAction::triggered,
                    this, &MainWindow::switchToConstruct),
            connect(ui->actionArsenal, &QAction::triggered,
                    &engine, &Client::switchToFactory),
            connect(ui->actionArsenal, &QAction::triggered,
                    this, &MainWindow::switchToArsenal),
            connect(ui->actionAnchorage, &QAction::triggered,
                    &engine, &Client::switchToFactory),
            connect(ui->actionAnchorage, &QAction::triggered,
                    this, &MainWindow::switchToAnchorage),
            connect(ui->actionShip_Blueprints, &QAction::triggered,
                    &engine, &Client::switchToFactory),
            connect(ui->actionShip_Blueprints, &QAction::triggered,
                    this, &MainWindow::switchToBlueprint),
            connect(ui->actionBattle, &QAction::triggered,
                    &engine, &Client::switchToBattleView),
            connect(ui->actionBattle, &QAction::triggered,
                    this, &MainWindow::switchToSortie),
            connect(ui->actionResource_status, &QAction::triggered,
                    &engine, &Client::switchToBattleView),
            connect(ui->actionResource_status, &QAction::triggered,
                    this, &MainWindow::showResourceGain),
            connect(ui->actionCompose, &QAction::triggered,
                    &engine, &Client::switchToFleetView),
            connect(ui->actionCompose, &QAction::triggered,
                    this, &MainWindow::switchToFleet),
            connect(ui->actionIndustrial_Plant, &QAction::triggered,
                    &engine, &Client::switchToFactory),
            connect(ui->actionIndustrial_Plant, &QAction::triggered,
                    this, &MainWindow::switchToRank),
            connect(ui->actionCloning_Vats, &QAction::triggered,
                    &engine, &Client::switchToFactory),
            connect(ui->actionCloning_Vats, &QAction::triggered,
                    this, &MainWindow::switchToCloningVats),
        };
}

void MainWindow::adjust(int) {
    QTimer::singleShot(0, this, [this]() {
        adjustArea(lay->currentWidget(), ui->MainArea->frameSize());
    });
}

void MainWindow::adjustArea(QWidget *input, const QSize &size) {
    input->move(0, 0);
    input->resize(size);
    update();
}

void MainWindow::buyOrdResources() {
    BuyOrdResourcesDialog dialog(this);
    dialog.exec();
}

void MainWindow::buyARD() {
    ARDCouponDialog dialog(this);
    dialog.exec();
}

void MainWindow::buyEquip() {
    BuyEquipDialog dialog(this);
    dialog.exec();
}

void MainWindow::buyMedal() {
    MedalBuyDialog dialog(this);
    dialog.exec();
}

void MainWindow::factoryRefresh() {
    QString cmd1 = QStringLiteral("refresh Factory");
    Client &engine = Client::getInstance();
    engine.parse(cmd1);
}

void MainWindow::gamestateInit() {
    /*
    gamestateChanged(KP::Offline);
    adjustArea(newLoginScreen,
               ui->MainArea->frameSize());
    update();
*/
}

void MainWindow::gamestateChanged(KP::GameState state) {
    Client &engine = Client::getInstance();
    if(engine.isInBattle() ^ (state == KP::BattleMapView)) {
        /* lock */
        return;
    }
    state == KP::Offline ? ui->ResourcesBar->hide()
                         : ui->ResourcesBar->show();
    ui->menuShop->setEnabled(state != KP::Offline);
    switch(state) {
    case KP::Offline: lay->setCurrentWidget(newLoginScreen); break;
    case KP::Port: lay->setCurrentWidget(portArea); break;
    case KP::Factory: lay->setCurrentWidget(factoryArea); break;
    case KP::TechView: lay->setCurrentWidget(techArea); break;
    case KP::BattleMapView: lay->setCurrentWidget(battleArea);
        battleArea->switchToState(KP::MapDetail); break;
    case KP::SortieMapView: lay->setCurrentWidget(battleArea);
        battleArea->switchToState(KP::MapView); break;
    case KP::FleetView: lay->setCurrentWidget(fleetArea); break;
    case KP::RepairView: lay->setCurrentWidget(repairArea); break;
    }
    adjustArea(lay->currentWidget(), ui->MainArea->frameSize());
}

void MainWindow::printMessage(QString text, QColor background,
                              QColor foreground) {
    ui->LogBrowser->setTextBackgroundColor(background);
    ui->LogBrowser->setTextColor(foreground);
    ui->LogBrowser->append(text);
    ui->LogBrowser->verticalScrollBar()->setValue(
        ui->LogBrowser->verticalScrollBar()->maximum());
}

void MainWindow::processCmd() {
    emit cmdMessage(ui->CommandPrompt->toPlainText());
    ui->CommandPrompt->clear();
}

void MainWindow::switchToAnchorage() {
    Client &engine = Client::getInstance();
    if(!engine.loggedIn()) {
        return;
    }
    factoryArea->setState(KP::Anchorage);
    factoryArea->switchToState();
    adjust();
}

void MainWindow::switchToArsenal() {
    Client &engine = Client::getInstance();
    if(!engine.loggedIn()) {
        return;
    }
    factoryArea->setState(KP::Arsenal);
    factoryArea->switchToState();
    adjust();
}

void MainWindow::switchToBlueprint() {
    Client &engine = Client::getInstance();
    if(!engine.loggedIn()) {
        return;
    }
    factoryArea->setState(KP::BlueprintView);
    factoryArea->switchToState();
    adjust();
}

void MainWindow::switchToCloningVats() {
    Client &engine = Client::getInstance();
    if(!engine.loggedIn()) {
        return;
    }
    factoryArea->setState(KP::CloningVats);
    factoryArea->switchToState();
    adjust();
}

void MainWindow::switchToConstruct() {
    Client &engine = Client::getInstance();
    if(!engine.loggedIn()) {
        return;
    }
    factoryArea->setState(KP::Construction);
    factoryArea->switchToState();
    adjust();
}

void MainWindow::switchToDevelop() {
    Client &engine = Client::getInstance();
    if(!engine.loggedIn()) {
        return;
    }
    factoryArea->setState(KP::Development);
    factoryArea->switchToState();
    adjust();
}

void MainWindow::switchToFleet() {
    Client &engine = Client::getInstance();
    if(!engine.loggedIn()) {
        return;
    }
    adjust();
    /* TODO:ADD */
}

void MainWindow::switchToRank() {
    Client &engine = Client::getInstance();
    if(!engine.loggedIn()) {
        return;
    }
    factoryArea->setState(KP::RankView);
    factoryArea->switchToState();
    adjust();
}

void MainWindow::showResourceGain() {
    Client &engine = Client::getInstance();
    if(!engine.loggedIn()) {
        return;
    }
    battleArea->switchToState(KP::ResourceGainView);
    adjust();
}

void MainWindow::switchToSortie() {
    Client &engine = Client::getInstance();
    if(!engine.loggedIn()) {
        return;
    }
    battleArea->switchToState(KP::MapView);
    engine.demandMapSupremacy();
    adjust();
}

void MainWindow::updateColorScheme(Qt::ColorScheme colorscheme) {
    QPalette pal = QGuiApplication::palette();
    setStyleSheet("QMenuBar { background-color: palette(button)}");
    /* https://www.w3.org/TR/SVG11/types.html#ColorKeywords */
    switch(colorscheme) {
    case Qt::ColorScheme::Dark:
#if defined(Q_OS_WIN)
        pal.setColor(QPalette::Window, QColor::fromString("midnightblue"));
        pal.setColor(QPalette::Base, QColor::fromString("midnightblue"));
#else
        pal.setColor(QPalette::Window, QColor::fromString("midnightblue"));
        pal.setColor(QPalette::Base, QColor::fromString("midnightblue"));
#endif
        break;
    case Qt::ColorScheme::Light: [[fallthrough]];
    default:
        pal.setColor(QPalette::Window, QColor::fromString("lightskyblue"));
        pal.setColor(QPalette::Base, QColor::fromString("lightskyblue"));
        break;
    }
    QApplication::setPalette(pal);
}

void MainWindow::updateResources(const QJsonObject &djson) {
    ui->OilCount->setText(QString::number(djson["oil"].toInt()));
    ui->ExploCount->setText(QString::number(djson["explo"].toInt()));
    ui->SteelCount->setText(QString::number(djson["steel"].toInt()));
    ui->RubberCount->setText(QString::number(djson["rubber"].toInt()));
    ui->AluminumCount->setText(QString::number(djson["al"].toInt()));
    ui->TungstenCount->setText(QString::number(djson["w"].toInt()));
    ui->ChromiumCount->setText(QString::number(djson["cr"].toInt()));
}

void MainWindow::showEvent(QShowEvent *event) {
    if (m_firstShow) {
        m_firstShow = false;
        QScreen *currentScreen = screen();
        if (currentScreen) {
            move(currentScreen->availableGeometry().center() - rect().center());
        }
    }
    QMainWindow::showEvent(event);
}

/* reimplement */
void MainWindow::resizeEvent(QResizeEvent *event) {
    adjustArea(lay->currentWidget(), ui->MainArea->frameSize());
    QWidget::resizeEvent(event);
}
