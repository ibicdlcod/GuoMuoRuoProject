#include "NewLoginS.h"
#include "ui_NewLoginS.h"
#include <QDir>
#include <QSettings>

extern std::unique_ptr<QSettings> settings;

NewLoginS::NewLoginS(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::NewLoginS)
{
    ui->setupUi(this);
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
    QObject::connect(ui->ContinueButton, &QPushButton::clicked,
                     this, &NewLoginS::complete);
}

NewLoginS::~NewLoginS()
{
    delete ui;
}

void NewLoginS::complete() {
    emit showLicenseComplete();
}
