#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDir>
#include <QSettings>
#include <QResizeEvent>
#include <QScreen>
#include "keyenterreceiver.h"
#include "../clientv2.h"

extern std::unique_ptr<QSettings> settings;

MainWindow::MainWindow(QWidget *parent, int argc, char ** argv)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    /* this is done instead of in *.ui for it does not cascade */
    ui->MainArea->setObjectName("mainArea");
    ui->MainArea->setStyleSheet("QWidget#mainArea { border-style: none }");

    move(screen()->geometry().center() - frameGeometry().center());
    ui->PortArea->hide();
    ui->FactoryArea->hide();
    ui->LoginScreen->hide();
    ui->TechArea->hide();

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
    connect(ui->actionLogout, &QAction::triggered,
            &engine, &Clientv2::parseDisconnectReq);
    connect(ui->actionExit, &QAction::triggered,
            &engine, &Clientv2::parseQuit);
    connect(&engine, &Clientv2::receivedResourceInfo,
            this, &MainWindow::updateResources);

    portArea = new PortArea(ui->PortArea);
    licenseArea = new LicenseArea(ui->License);
    newLoginScreen = new NewLoginS(ui->LoginScreen);
    factoryArea = new FactoryArea(ui->FactoryArea);
    techArea = new TechView(ui->TechArea);
    QTimer::singleShot(1, this,
                       [this]
                       {adjustArea(licenseArea,
                                    ui->License->frameSize());});
    connect(licenseArea, &LicenseArea::showLicenseComplete,
            ui->License, &QWidget::hide);
    connect(licenseArea, &LicenseArea::showLicenseComplete,
            this, &MainWindow::gamestateInit);
    connect(licenseArea, &LicenseArea::showLicenseComplete,
            newLoginScreen, &QWidget::show);
    QTimer::singleShot(settings->value("client/licenseareapersist",
                                       5000).toInt(), this,
                       [this]{
                           this->licenseArea->complete();
                       });
}

MainWindow::~MainWindow()
{
    delete portArea;
    delete licenseArea;
    delete newLoginScreen;
    delete factoryArea;
    delete techArea;
    delete ui;
}

void MainWindow::adjustArea(QFrame *input, const QSize &size) {
    QTimer::singleShot(1, this,
                       [input, size]{input->resize(size);});
}

void MainWindow::gamestateInit() {
    gamestateChanged(KP::Offline);
    QTimer::singleShot(1, this,
                       [this]
                       {adjustArea(newLoginScreen,
                                    ui->LoginScreen->frameSize());});
}

void MainWindow::gamestateChanged(KP::GameState state) {
    state == KP::Offline ? ui->LoginScreen->show()
                         : ui->LoginScreen->hide();
    state == KP::Offline ? ui->ResourcesBar->hide()
                         : ui->ResourcesBar->show();
    state == KP::Port ? (
        licenseArea->neverComplete(),
        ui->PortArea->show(),
        QTimer::singleShot(1, this,
                           [this]
                           {adjustArea(portArea,
                                        ui->PortArea->frameSize());}))
                      : ui->PortArea->hide();
    state == KP::Factory ? (ui->FactoryArea->show(), factoryRefresh(),
                            factoryArea->resize(ui->FactoryArea->size())) :
        ui->FactoryArea->hide();
    state == KP::TechView ? (
        ui->TechArea->show(),
        QTimer::singleShot(1, this,
                           [this]
                           {adjustArea(techArea,
                                        ui->TechArea->frameSize());}))
                          : ui->TechArea->hide();
}

void MainWindow::printMessage(QString text, QColor background,
                              QColor foreground) {
    ui->LogBrowser->setTextBackgroundColor(background);
    ui->LogBrowser->setTextColor(foreground);
    ui->LogBrowser->append(text);
}

void MainWindow::processCmd() {
    emit cmdMessage(ui->CommandPrompt->toPlainText());
    ui->CommandPrompt->clear();
}

void MainWindow::factoryRefresh() {
    QString cmd1 = QStringLiteral("refresh Factory");
    Clientv2 &engine = Clientv2::getInstance();
    engine.parse(cmd1);
}

void MainWindow::switchToConstruct() {
    Clientv2 &engine = Clientv2::getInstance();
    if(!engine.loggedIn()) {
        return;
    }
    factoryArea->setDevelop(KP::Construction);
    factoryArea->switchToDevelop();
}

void MainWindow::switchToDevelop() {
    Clientv2 &engine = Clientv2::getInstance();
    if(!engine.loggedIn()) {
        return;
    }
    factoryArea->setDevelop(KP::Development);
    factoryArea->switchToDevelop();
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
    if(!ui->PortArea->isHidden()) {
        adjustArea(portArea, ui->PortArea->size());
    }
    if(!ui->License->isHidden()) {
        adjustArea(licenseArea, ui->License->size());
    }
    if(!ui->LoginScreen->isHidden()) {
        adjustArea(newLoginScreen, ui->LoginScreen->size());
    }
    if(!ui->FactoryArea->isHidden()) {
        adjustArea(factoryArea, ui->FactoryArea->size());
    }
    if(!ui->TechArea->isHidden()) {
        adjustArea(techArea, ui->TechArea->size());
    }
    QWidget::resizeEvent(event);
}
