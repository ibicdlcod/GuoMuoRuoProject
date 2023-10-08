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
    QObject::connect(&engine, &Clientv2::receivedFactoryRefresh,
                     this, &MainWindow::doFactoryRefresh);
    slotfs.append(ui->Factory_Slot_0);
    slotfs.append(ui->Factory_Slot_1);
    slotfs.append(ui->Factory_Slot_2);
    slotfs.append(ui->Factory_Slot_3);
    slotfs.append(ui->Factory_Slot_4);
    slotfs.append(ui->Factory_Slot_5);
    slotfs.append(ui->Factory_Slot_6);
    slotfs.append(ui->Factory_Slot_7);
    slotfs.append(ui->Factory_Slot_8);
    slotfs.append(ui->Factory_Slot_9);
    slotfs.append(ui->Factory_Slot_10);
    slotfs.append(ui->Factory_Slot_11);
    slotfs.append(ui->Factory_Slot_12);
    slotfs.append(ui->Factory_Slot_13);
    slotfs.append(ui->Factory_Slot_14);
    slotfs.append(ui->Factory_Slot_15);
    slotfs.append(ui->Factory_Slot_16);
    slotfs.append(ui->Factory_Slot_17);
    slotfs.append(ui->Factory_Slot_18);
    slotfs.append(ui->Factory_Slot_19);
    slotfs.append(ui->Factory_Slot_20);
    slotfs.append(ui->Factory_Slot_21);
    slotfs.append(ui->Factory_Slot_22);
    slotfs.append(ui->Factory_Slot_23);
    for(auto iter = slotfs.begin(); iter < slotfs.end(); ++iter) {
        QObject::connect((*iter), &FactorySlot::clickedSpec,
                         this, &MainWindow::developClicked);
        (*iter)->setSlotnum(iter - slotfs.begin());
        (*iter)->setStatus();
    }

    portArea = new PortArea(ui->PortArea);
    licenseArea = new LicenseArea(ui->License);
    QTimer::singleShot(1, this, &MainWindow::adjustLicenseArea);
}

MainWindow::~MainWindow()
{
    delete portArea;
    delete ui;
}

void MainWindow::developClicked(bool checked, int slotnum) {
    Q_UNUSED(checked)
    DevelopWindow w;
    if(w.exec() == QDialog::Rejected)
        qDebug() << "FUCK" << slotnum << Qt::endl;
    else {
        Clientv2 &engine = Clientv2::getInstance();
        QString msg = QStringLiteral("develop %1 %2")
                .arg(w.EquipIdDesired()).arg(slotnum);
        qDebug() << msg;
        engine.parse(msg);
    }
}

void MainWindow::doFactoryRefresh(const QJsonObject &input) {
    QJsonArray content = input["content"].toArray();
    for(int i = 0; i < content.size(); ++i) {
        slotfs[i]->setOpen(true);
        QJsonObject item = content[i].toObject();
        if(!item["done"].toBool()) {
            slotfs[i]->setCompleteTime(
                        QDateTime::fromString(
                            item["completetime"].toString()));
        } else {
            slotfs[i]->setComplete(true);
        }
        slotfs[i]->setStatus();
    }
}

void MainWindow::gamestateChanged(KP::GameState state) {
    state == KP::Offline ? ui->LoginScreen->show() :
                           ui->LoginScreen->hide();
    if(state == KP::Port) {
        ui->PortArea->show();
        portArea->resize(ui->PortArea->width(),ui->PortArea->height());
    }
    else {
        ui->PortArea->hide();
    }
    state == KP::Factory ? (ui->FactoryArea->show(), factoryRefresh()) :
                           ui->FactoryArea->hide();

}

void MainWindow::parseConnectReq() {
    QStringList cmd1 = {QStringLiteral("connect"),
                        ui->ServerEdit->text(),
                        ui->PortEdit->text()
                       };
    QString cmd1Comb = cmd1.join(" ");
    Clientv2 &engine = Clientv2::getInstance();
    engine.parse(cmd1Comb);
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
    ui->FactoryLabel->setText(qtTrId("develop-equipment"));
}

void MainWindow::adjustLicenseArea() {
    qDebug() << "CHANGE";
    qDebug() << ui->License->frameSize().width();
    qDebug() << ui->License->frameSize().height();
    licenseArea->resize(ui->License->frameSize());
}

/* reimplement */
void MainWindow::resizeEvent(QResizeEvent *event)
{
    if(!ui->PortArea->isHidden()) {
        portArea->resize(ui->PortArea->width(),ui->PortArea->height());
    }
    QWidget::resizeEvent(event);
}
