#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDir>
#include <QSettings>
#include <QResizeEvent>
#include <QScreen>
#include "keyenterreceiver.h"

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

    KeyEnterReceiver *key = new KeyEnterReceiver();
    ui->CommandPrompt->installEventFilter(key);

    Clientv2 &engine = Clientv2::getInstance();

    QObject::connect(this, &MainWindow::cmdMessage,
                     &engine, &Clientv2::parse);
    QObject::connect(&engine, &Clientv2::qout,
                     this, &MainWindow::printMessage);
    QObject::connect(&engine, &Clientv2::aboutToQuit,
                     this, &MainWindow::close);
    QObject::connect(key, &KeyEnterReceiver::enterPressed,
                     this, &MainWindow::processCmd);
    QObject::connect(&engine, &Clientv2::gamestateChanged,
                     this, &MainWindow::gamestateChanged);
    QObject::connect(ui->actionBack_to_naval_base, &QAction::triggered,
                     &engine, &Clientv2::backToNavalBase);
    QObject::connect(ui->actionView_Tech, &QAction::triggered,
                     &engine, &Clientv2::switchToTech);
    QObject::connect(ui->actionDevelop_Equipment, &QAction::triggered,
                     &engine, &Clientv2::switchToFactory);
    QObject::connect(ui->actionDevelop_Equipment, &QAction::triggered,
                     this, &MainWindow::switchToDevelop);
    QObject::connect(ui->actionConstruct_Ships, &QAction::triggered,
                     &engine, &Clientv2::switchToFactory);
    QObject::connect(ui->actionConstruct_Ships, &QAction::triggered,
                     this, &MainWindow::switchToConstruct);
    QObject::connect(ui->actionLogout, &QAction::triggered,
                     &engine, &Clientv2::parseDisconnectReq);
    QObject::connect(ui->actionExit, &QAction::triggered,
                     &engine, &Clientv2::parseQuit);

    portArea = new PortArea(ui->PortArea);
    licenseArea = new LicenseArea(ui->License);
    newLoginScreen = new NewLoginS(ui->LoginScreen);
    factoryArea = new FactoryArea(ui->FactoryArea);
    techArea = new TechView(ui->TechArea);
    QTimer::singleShot(1, this,
                       [this]
                       {adjustArea(licenseArea,
                                    ui->License->frameSize());});
    QObject::connect(licenseArea, &LicenseArea::showLicenseComplete,
                     ui->LoginScreen, &QWidget::show);
    QObject::connect(licenseArea, &LicenseArea::showLicenseComplete,
                     ui->License, &QWidget::hide);
    QObject::connect(licenseArea, &LicenseArea::showLicenseComplete,
                     this, &MainWindow::gamestateInit);
    QObject::connect(licenseArea, &LicenseArea::showLicenseComplete,
                     newLoginScreen, &QWidget::show);
    QTimer::singleShot(settings->value("client/license_area_persist",
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
    state == KP::Port ? (
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
    factoryArea->setDevelop(false);
    factoryArea->switchToDevelop();
}

void MainWindow::switchToDevelop() {
    Clientv2 &engine = Clientv2::getInstance();
    if(!engine.loggedIn()) {
        return;
    }
    factoryArea->setDevelop(true);
    factoryArea->switchToDevelop();
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
