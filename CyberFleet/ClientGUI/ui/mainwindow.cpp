#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDir>
#include <QSettings>
#include <QResizeEvent>
#include "developwindow.h"
#include "keyenterreceiver.h"

extern std::unique_ptr<QSettings> settings;

MainWindow::MainWindow(QWidget *parent, int argc, char ** argv)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->PortArea->hide();
    ui->FactoryArea->hide();
    ui->LoginScreen->hide();

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
    QObject::connect(ui->actionDevelop_Equipment, &QAction::triggered,
                     &engine, &Clientv2::switchToFactory);
    QObject::connect(ui->actionDevelop_Equipment, &QAction::triggered,
                     this, &MainWindow::switchToDevelop);
    QObject::connect(ui->actionLogout, &QAction::triggered,
                     &engine, &Clientv2::parseDisconnectReq);
    QObject::connect(ui->actionExit, &QAction::triggered,
                     &engine, &Clientv2::parseQuit);

    portArea = new PortArea(ui->PortArea);
    licenseArea = new LicenseArea(ui->License);
    newLoginScreen = new NewLoginS(ui->LoginScreen);
    factoryArea = new FactoryArea(ui->FactoryArea);
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
}

MainWindow::~MainWindow()
{
    delete portArea;
    delete licenseArea;
    delete ui;
}

void MainWindow::adjustArea(QFrame *input, const QSize &size) {
    input->resize(size);
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
                                        ui->PortArea->frameSize());})
        )
                      : ui->PortArea->hide();
    state == KP::Factory ? (ui->FactoryArea->show(), factoryRefresh(),
                            factoryArea->resize(ui->FactoryArea->size())) :
        ui->FactoryArea->hide();
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

void MainWindow::switchToDevelop() {
    Clientv2 &engine = Clientv2::getInstance();
    if(!engine.loggedIn()) {
        return;
    }
    factoryState = KP::Development;
    //% "Develop Equipment"
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
    QWidget::resizeEvent(event);
}
