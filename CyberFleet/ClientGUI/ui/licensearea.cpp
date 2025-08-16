#include "licensearea.h"
#include "ui_licensearea.h"
#include <QDir>
#include <QSettings>

extern std::unique_ptr<QSettings> settings;

LicenseArea::LicenseArea(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::LicenseArea)
{
    ui->setupUi(this);

    /* this is done instead of in *.ui for it does not cascade */
    ui->LicenseText->setObjectName("licenseText");
    ui->LicenseText->setStyleSheet("QTextBrowser#licenseText { border-style: none; }");
    ui->BelowLicense->setObjectName("belowLicense");
    ui->BelowLicense->setStyleSheet("QFrame#belowLicense { border-style: none }");
    ui->Naganami->setObjectName("naganami");
    ui->Naganami->setStyleSheet("QTextBrowser#naganami { border-style: none }");

    QString notice;
    QDir currentDir = QDir::current();
    /* the default is qt resource system */
    QString openingwords = settings->value("license_notice",
                                           ":/openingwords.txt").toString();
    QFile licenseFile(currentDir.filePath(openingwords));
    if(Q_UNLIKELY(!licenseFile.open(QIODevice::ReadOnly | QIODevice::Text))) {
        //% "Can't find license file, exiting."
        qFatal() << qtTrId("licence-not-found").toUtf8();
    }
    else {
        QTextStream instream1(&licenseFile);
        ui->LicenseText->setAlignment(Qt::AlignCenter);
        notice = instream1.readAll();
    }
    ui->LicenseText->setText(notice);
    ui->LicenseText->selectAll();
    ui->LicenseText->setAlignment(Qt::AlignCenter);
    auto textCursor = ui->LicenseText->textCursor();
    textCursor.clearSelection();
    ui->LicenseText->setTextCursor(textCursor);
    //% "What? Admiral Tanaka? He's the real deal, isn't he?\nGreat at battle and bad at politics--so cool!"
    ui->Naganami->setText(qtTrId("naganami-words"));
    ui->Naganami->selectAll();
    ui->Naganami->setAlignment(Qt::AlignCenter);
    textCursor = ui->Naganami->textCursor();
    textCursor.clearSelection();
    ui->Naganami->setTextCursor(textCursor);
    //% "Continue"
    ui->ContinueButton->setText(qtTrId("license-continue"));
    connect(ui->ContinueButton, &QPushButton::clicked,
                     this, &LicenseArea::complete);
}

LicenseArea::~LicenseArea()
{
    delete ui;
}

void LicenseArea::complete() {
    if(!completeOff)
        emit showLicenseComplete();
}

void LicenseArea::neverComplete() {
    completeOff = true;
}
