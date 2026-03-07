/* Copyright (C) 2026 Harusoft Inc.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDir>
#include <QSettings>
#include <QResizeEvent>
#include <QScreen>
#include <QStyleHints>
#include <QScrollBar>
#include <QMessageBox>
#include "keyenterreceiver.h"
#include "../clientv2.h"

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

    move(screen()->geometry().center() - frameGeometry().center());

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

    KeyEnterReceiver *key = new KeyEnterReceiver();
    ui->CommandPrompt->installEventFilter(key);

    Clientv2 &engine = Clientv2::getInstance();

    unlockBattle();
    connect(&engine, &Clientv2::lockBattle,
            this, &MainWindow::lockBattle);
    connect(&engine, &Clientv2::unlockBattle,
            this, &MainWindow::unlockBattle);
    connect(this, &MainWindow::cmdMessage,
            &engine, &Clientv2::parse);
    connect(&engine, &Clientv2::qout,
            this, &MainWindow::printMessage);
    connect(&engine, &Clientv2::aboutToQuit,
            this, &MainWindow::close);
    connect(key, &KeyEnterReceiver::enterPressed,
            this, &MainWindow::processCmd);
    connect(&engine, &Clientv2::gamestateChanged,
            this, &MainWindow::gamestateChanged);
    connect(ui->actionLogout, &QAction::triggered,
            &engine, &Clientv2::parseDisconnectReq);
    connect(ui->actionExit, &QAction::triggered,
            &engine, &Clientv2::parseQuit);
    connect(&engine, &Clientv2::receivedResourceInfo,
            this, &MainWindow::updateResources);
    connect(ui->actionSettings, &QAction::triggered,
            &settingsWindow, &QDialog::show);
    connect(ui->actionLicense, &QAction::triggered,
            this, [this](){
                QMessageBox box(this);
                QString notice;
                QDir currentDir = QDir::current();
                QString openingwords = settings->value("license_notice",
                                                       ":/openingwords.txt").toString();
                QFile licenseFile(currentDir.filePath(openingwords));
                if(Q_UNLIKELY(!licenseFile.open(QIODevice::ReadOnly | QIODevice::Text))) {
                    //% "Can't find license file, exiting."
                    qFatal() << qtTrId("licence-not-found").toUtf8();
                }
                else {
                    QTextStream instream1(&licenseFile);
                    notice = instream1.readAll();
                }
                notice.replace("<https://www.gnu.org/licenses/>",
                               "<a href=\"https://www.gnu.org/licenses\">the GNU.org page</a>");
                notice.replace("\n", "<br>");
                notice = "<p align='center'>" + notice + "</p>";
                box.setText(notice);
                box.exec();
            });
    connect(ui->actionAbout_Qt, &QAction::triggered,
            QApplication::instance(), &QApplication::aboutQt);

    portArea = new PortArea();
    licenseArea = new LicenseArea();
    newLoginScreen = new NewLoginS();
    factoryArea = new FactoryArea();
    techArea = new TechView();
    battleArea = new Sortie();
    fleetArea = new FleetView();

    lay->addWidget(portArea);
    lay->addWidget(licenseArea);
    lay->addWidget(newLoginScreen);
    lay->addWidget(factoryArea);
    lay->addWidget(techArea);
    lay->addWidget(battleArea);
    lay->addWidget(fleetArea);

    ui->MainArea->setLayout(lay);
    lay->setContentsMargins(0,0,0,0);

    QTimer::singleShot(100, this,
                       [this]
                       {
                           lay->setCurrentWidget(licenseArea);
                           adjustArea(licenseArea,
                                      ui->MainArea->frameSize());
                       });
    connect(licenseArea, &LicenseArea::showLicenseComplete,
            lay, [this](){lay->setCurrentWidget(newLoginScreen);});
    connect(licenseArea, &LicenseArea::showLicenseComplete,
            this, &MainWindow::gamestateInit);
    QTimer::singleShot(settings->value("client/licenseareapersist",
                                       5000).toInt(), this,
                       [this]{
                           this->licenseArea->complete();
                       });
    connect(&engine, &Clientv2::equipRegistryComplete,
            portArea, &PortArea::equipRegistryComplete);
    connect(&engine, &Clientv2::shipRegistryComplete,
            portArea, &PortArea::shipRegistryComplete);
    connect(&engine, &Clientv2::mapRegistryComplete,
            portArea, &PortArea::mapRegistryComplete);
    connect(&engine, &Clientv2::tsunkitAssetsComplete,
            portArea, &PortArea::hello);

    connect(lay, &QStackedLayout::currentChanged,
            this, &MainWindow::adjust);
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
    /* must ensure consistency with below */
    Clientv2 &engine = Clientv2::getInstance();
    disconnect(ui->actionBack_to_naval_base, &QAction::triggered,
               &engine, &Clientv2::backToNavalBase);
    disconnect(ui->actionView_Tech, &QAction::triggered,
               &engine, &Clientv2::switchToTech);
    disconnect(ui->actionDevelop_Equipment, &QAction::triggered,
               &engine, &Clientv2::switchToFactory);
    disconnect(ui->actionDevelop_Equipment, &QAction::triggered,
               this, &MainWindow::switchToDevelop);
    disconnect(ui->actionConstruct_Ships, &QAction::triggered,
               &engine, &Clientv2::switchToFactory);
    disconnect(ui->actionConstruct_Ships, &QAction::triggered,
               this, &MainWindow::switchToConstruct);
    disconnect(ui->actionArsenal, &QAction::triggered,
               &engine, &Clientv2::switchToFactory);
    disconnect(ui->actionArsenal, &QAction::triggered,
               this, &MainWindow::switchToArsenal);
    disconnect(ui->actionAnchorage, &QAction::triggered,
               &engine, &Clientv2::switchToFactory);
    disconnect(ui->actionAnchorage, &QAction::triggered,
               this, &MainWindow::switchToAnchorage);
    disconnect(ui->actionShip_Blueprints, &QAction::triggered,
               &engine, &Clientv2::switchToFactory);
    disconnect(ui->actionShip_Blueprints, &QAction::triggered,
               this, &MainWindow::switchToBlueprint);
    disconnect(ui->actionBattle, &QAction::triggered,
               &engine, &Clientv2::switchToBattleView);
    disconnect(ui->actionBattle, &QAction::triggered,
               this, &MainWindow::switchToSortie);
    disconnect(ui->actionCompose, &QAction::triggered,
               &engine, &Clientv2::switchToFleetView);
    disconnect(ui->actionCompose, &QAction::triggered,
               this, &MainWindow::switchToFleet);
}

void MainWindow::unlockBattle() {
    Clientv2 &engine = Clientv2::getInstance();
    connect(ui->actionBack_to_naval_base, &QAction::triggered,
            &engine, &Clientv2::backToNavalBase);
    connect(ui->actionView_Tech, &QAction::triggered,
            &engine, &Clientv2::switchToTech);
    connect(ui->actionDevelop_Equipment, &QAction::triggered,
            &engine, &Clientv2::switchToFactory);
    connect(ui->actionDevelop_Equipment, &QAction::triggered,
            this, &MainWindow::switchToDevelop);
    connect(ui->actionConstruct_Ships, &QAction::triggered,
            &engine, &Clientv2::switchToFactory);
    connect(ui->actionConstruct_Ships, &QAction::triggered,
            this, &MainWindow::switchToConstruct);
    connect(ui->actionArsenal, &QAction::triggered,
            &engine, &Clientv2::switchToFactory);
    connect(ui->actionArsenal, &QAction::triggered,
            this, &MainWindow::switchToArsenal);
    connect(ui->actionAnchorage, &QAction::triggered,
            &engine, &Clientv2::switchToFactory);
    connect(ui->actionAnchorage, &QAction::triggered,
            this, &MainWindow::switchToAnchorage);
    connect(ui->actionShip_Blueprints, &QAction::triggered,
            &engine, &Clientv2::switchToFactory);
    connect(ui->actionShip_Blueprints, &QAction::triggered,
            this, &MainWindow::switchToBlueprint);
    connect(ui->actionBattle, &QAction::triggered,
            &engine, &Clientv2::switchToBattleView);
    connect(ui->actionBattle, &QAction::triggered,
            this, &MainWindow::switchToSortie);
    connect(ui->actionCompose, &QAction::triggered,
            &engine, &Clientv2::switchToFleetView);
    connect(ui->actionCompose, &QAction::triggered,
            this, &MainWindow::switchToFleet);
}

void MainWindow::adjust(int) {
    QTimer::singleShot(10, this,
                       [this]
                       {
                           auto geo = geometry();
                           auto tempGeo = geo;
                           tempGeo.setRect(tempGeo.x(), tempGeo.y(),
                                           tempGeo.width(), tempGeo.height()-1);
                           setGeometry(tempGeo);
                           setGeometry(geo);
                       });
}

void MainWindow::adjustArea(QWidget *input, const QSize &size) {
    input->move(0, 0);
    input->resize(size);
    update();
}

void MainWindow::factoryRefresh() {
    QString cmd1 = QStringLiteral("refresh Factory");
    Clientv2 &engine = Clientv2::getInstance();
    engine.parse(cmd1);
}

void MainWindow::gamestateInit() {
    gamestateChanged(KP::Offline);
    adjustArea(newLoginScreen,
               ui->MainArea->frameSize());
    update();
}

void MainWindow::gamestateChanged(KP::GameState state) {
    Clientv2 &engine = Clientv2::getInstance();
    if(engine.isInBattle() ^ (state == KP::BattleMapView)) {
        qCritical() << "FUCK";
        /* lock */
        return;
    }
    state == KP::Offline ? ui->ResourcesBar->hide()
                         : ui->ResourcesBar->show();
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
    Clientv2 &engine = Clientv2::getInstance();
    if(!engine.loggedIn()) {
        return;
    }
    factoryArea->setState(KP::Anchorage);
    factoryArea->switchToState();
    adjust();
}

void MainWindow::switchToArsenal() {
    Clientv2 &engine = Clientv2::getInstance();
    if(!engine.loggedIn()) {
        return;
    }
    factoryArea->setState(KP::Arsenal);
    factoryArea->switchToState();
    adjust();
}

void MainWindow::switchToBlueprint() {
    Clientv2 &engine = Clientv2::getInstance();
    if(!engine.loggedIn()) {
        return;
    }
    factoryArea->setState(KP::BlueprintView);
    factoryArea->switchToState();
    adjust();
}

void MainWindow::switchToConstruct() {
    Clientv2 &engine = Clientv2::getInstance();
    if(!engine.loggedIn()) {
        return;
    }
    factoryArea->setState(KP::Construction);
    factoryArea->switchToState();
    adjust();
}

void MainWindow::switchToDevelop() {
    Clientv2 &engine = Clientv2::getInstance();
    if(!engine.loggedIn()) {
        return;
    }
    factoryArea->setState(KP::Development);
    factoryArea->switchToState();
    adjust();
}

void MainWindow::switchToFleet() {
    Clientv2 &engine = Clientv2::getInstance();
    if(!engine.loggedIn()) {
        return;
    }
    adjust();
    /* TODO:ADD */
}

void MainWindow::switchToSortie() {
    Clientv2 &engine = Clientv2::getInstance();
    if(!engine.loggedIn()) {
        return;
    }
    battleArea->switchToState(KP::MapView);
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

/* reimplement */
void MainWindow::resizeEvent(QResizeEvent *event) {
    adjustArea(lay->currentWidget(), ui->MainArea->frameSize());
    QWidget::resizeEvent(event);
}
