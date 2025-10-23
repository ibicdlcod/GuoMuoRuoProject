#include "constructwindow.h"
#include "../../clientv2.h"
#include "ui_constructwindow.h"
#include "../../../Protocol/kp.h"
#include <QMetaEnum>

ConstructWindow::ConstructWindow(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ConstructWindow)
{
    ui->setupUi(this);
}

ConstructWindow::~ConstructWindow()
{
    delete ui;
}

void ConstructWindow::switchDisplay(const QString &nationality,
                                    const QString &shiptype,
                                    const QString &shipclass) {
    ui->shipname->clear();
    Clientv2 &engine = Clientv2::getInstance();
    bool pass = true;
    bool pass1 = false;
    //% "All ship types"
    QStringList typePasses = {qtTrId("all-shiptypes")};
    //% "All ship classes"
    QStringList classPasses = {qtTrId("all-shipclasses")};;
    static auto meta = QMetaEnum::fromType<KP::ShipNationality>();
    /*
    for(auto iter = engine.shipRegistryCache.keyValueBegin();
         iter != engine.shipRegistryCache.keyValueEnd();
         ++iter) {
        pass = true;
        if(!nationality.isEmpty() &&
            qtTrId(meta.key(iter->second->getNationality()))
                    .localeAwareCompare(nationality) != 0) {
            pass = false;
        }
        if(shiptype.isEmpty() && shipclass.isEmpty()) {
            if(pass) {
                QString type = iter->second->getType().toString();
                if(!typePasses.contains(type)) {
                    typePasses.append(type);
                }
            }
        }

        if(!shiptype.isEmpty() &&
            iter->second->getType().toString().localeAwareCompare(
                shiptype) != 0) {
            pass = false;
        }
        QString classText =
            iter->second->shipClassText[
                settings->value("client/language", "ja_JP").toString()
        ];
        if(classText.isEmpty()) {
            classText = iter->second->shipClassText["ja_JP"];
        }
        if(shipclass.isEmpty()) {
            if(pass) {
                if(!classPasses.contains(classText)) {
                    classPasses.append(classText);
                }
            }
        }

        if(!shipclass.isEmpty() && classText.localeAwareCompare(
                                        shipclass) != 0) {
            pass = false;
        }
        if(pass) {
            sortedShipIds.append(iter->first);
        }
    }
    if(searchTerm.isEmpty() && shiptype.isEmpty() && shipclass.isEmpty()) {
        emit typeBoxHint(typePasses);
    }
    if(searchTerm.isEmpty() && shipclass.isEmpty()) {
        emit classBoxHint(classPasses);
    }
    */
}

void ConstructWindow::initialize() {
    auto meta = QMetaEnum::fromType<KP::ShipNationality>();
    for(int i = 0; i < meta.keyCount(); ++i) {
        if(meta.value(i) == KP::ShipNationality::UnknownNation) {
            ui->shipnation->addItem(qtTrId("all-nationality"));
        }
        else {
            ui->shipnation->addItem(qtTrId(meta.key(i)));
        }
    }
}
