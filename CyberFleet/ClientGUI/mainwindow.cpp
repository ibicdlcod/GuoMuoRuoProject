#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDir>
#include <QSettings>
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
    QString notice;
    QDir currentDir = QDir::current();
    /* the default is qt resource system */
    QString openingwords = settings->value("license_notice",
                                           ":/openingwords.txt").toString();
    QFile licenseFile(currentDir.filePath(openingwords));
    if(Q_UNLIKELY(!licenseFile.open(QIODevice::ReadOnly | QIODevice::Text))) {
        //% "Can't find license file, exiting."
        qFatal(qtTrId("licence-not-found").toUtf8());
    }
    else {
        QTextStream instream1(&licenseFile);
        ui->LicenseText->setAlignment(Qt::AlignCenter);
        notice = instream1.readAll();
    }
    ui->LicenseText->setText(notice);
    ui->LicenseText->selectAll();
    ui->LicenseText->setAlignment(Qt::AlignCenter);
    ui->LicenseText->setTextColor(QColor("white"));
    auto textCursor = ui->LicenseText->textCursor();
    textCursor.clearSelection();
    ui->LicenseText->setTextCursor(textCursor);
    //% "What? Admiral Tanaka? He's the real deal, isn't he?\nGreat at battle and bad at politics--so cool!"
    ui->Naganami->setText(qtTrId("naganami-words"));
    ui->Naganami->selectAll();
    ui->Naganami->setAlignment(Qt::AlignCenter);
    ui->Naganami->setTextColor(QColor("white"));
    textCursor = ui->Naganami->textCursor();
    textCursor.clearSelection();
    ui->Naganami->setTextCursor(textCursor);
    //% "Continue"
    ui->ContinueButton->setText(qtTrId("license-continue"));

    KeyEnterReceiver *key = new KeyEnterReceiver();
    ui->CommandPrompt->installEventFilter(key);

    Clientv2 &engine = Clientv2::getInstance();

    QObject::connect(this, &MainWindow::receivedMessage,
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

    ui->PasswordEdit->setEchoMode(QLineEdit::Password);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::gamestateChanged(KP::GameState state) {
    state == KP::Offline ? ui->LoginScreen->show() :
                           ui->LoginScreen->hide();
    state == KP::Port ? ui->PortArea->show() :
                        ui->PortArea->hide();
    state == KP::Factory ? ui->FactoryArea -> show() :
                           ui->FactoryArea->hide();
}

void MainWindow::parseConnectReq() {
    QStringList cmd1 = {QStringLiteral("connect"),
                        ui->ServerEdit->text(),
                        ui->PortEdit->text(),
                        ui->UsernameEdit->text(),
                       };
    QString cmd1Comb = cmd1.join(" ");
    QString cmd2 = ui->PasswordEdit->text();
    Clientv2 &engine = Clientv2::getInstance();
    engine.parse(cmd1Comb);
    engine.parse(cmd2);
}

void MainWindow::parseRegReq() {
    if(!pwConfirmMode) {
        QStringList cmd1 = {QStringLiteral("register"),
                            ui->ServerEdit->text(),
                            ui->PortEdit->text(),
                            ui->UsernameEdit->text(),
                           };
        QString cmd1Comb = cmd1.join(" ");
        QString cmd2 = ui->PasswordEdit->text();
        Clientv2 &engine = Clientv2::getInstance();
        engine.parse(cmd1Comb);
        engine.parse(cmd2);
        ui->PasswordEdit->clear();
        //% "Confirm Password:"
        ui->label_4->setText(qtTrId("confirm-password"));
        pwConfirmMode = true;
    } else {
        QString cmd3 = ui->PasswordEdit->text();
        Clientv2 &engine = Clientv2::getInstance();
        engine.parse(cmd3);
        //% "Password:"
        ui->label_4->setText(qtTrId("password"));
        pwConfirmMode = false;
    }
}

void MainWindow::printMessage(QString text, QColor background,
                              QColor foreground) {
    ui->LogBrowser->setTextBackgroundColor(background);
    ui->LogBrowser->setTextColor(foreground);
    ui->LogBrowser->append(text);
}

void MainWindow::processCmd() {
    emit receivedMessage(ui->CommandPrompt->toPlainText());
    ui->CommandPrompt->clear();
}

void MainWindow::switchToDevelop() {
    Clientv2 &engine = Clientv2::getInstance();
    if(!engine.loggedIn()) {
        return;
    }
    factoryState = KP::Development;
    //% "Develop Equipment"
    ui->FactoryLabel->setText(qtTrId("develop-equipment"));
}
