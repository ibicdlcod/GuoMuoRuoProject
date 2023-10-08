#include "NewLoginS.h"
#include "clientv2.h"
#include "ui_NewLoginS.h"
#include <QDir>
#include <QSettings>

extern std::unique_ptr<QSettings> settings;

NewLoginS::NewLoginS(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::NewLoginS)
{
    ui->setupUi(this);
}

NewLoginS::~NewLoginS()
{
    delete ui;
}

void NewLoginS::parseConnectReq() {
    QStringList cmd1 = {
        QStringLiteral("connect"),
        ui->ServerEdit->text(),
        ui->PortEdit->text()
    };
    QString cmd1Comb = cmd1.join(" ");
    Clientv2 &engine = Clientv2::getInstance();
    engine.parse(cmd1Comb);
}
