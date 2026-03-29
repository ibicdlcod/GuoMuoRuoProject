/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "constructwindow.h"
#include "ui_constructwindow.h"

#include <QDialogButtonBox>
#include <QMessageBox>
#include <QMetaEnum>
#include <QUuid>

#include <cmath>

#include "../../../Protocol/kp.h"
#include "../../clientv2.h"

extern std::unique_ptr<QSettings> settings;

ConstructWindow::ConstructWindow(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ConstructWindow)
{
    ui->setupUi(this);
    connect(ui->shipnation, &QComboBox::currentIndexChanged,
            this, &ConstructWindow::switchDisplay);
    connect(ui->shiptype, &QComboBox::currentIndexChanged,
            this, &ConstructWindow::switchDisplay);
    connect(ui->shipclass, &QComboBox::currentIndexChanged,
            this, &ConstructWindow::switchDisplay);
    connect(ui->searchBox, &QTextEdit::textChanged,
            this, [this](){switchDisplay();});
    ui->searchBox->setStyleSheet(QStringLiteral(
        "background-color: palette(button);"
        ));
    connect(ui->shipname, &QComboBox::currentIndexChanged,
            this, &ConstructWindow::shipNameChanged);
    Client &engine = Client::getInstance();
    equipBoxes = {ui->equip1, ui->equip2, ui->equip3, ui->equip4, ui->equip5};
    assert(equipBoxes.size() == KP::maxEquipSlots);
    for(int i = 0; i < KP::maxEquipSlots; ++i) {
        equipBoxes[i]->setModel(engine.specModels[i]);
    }
}

ConstructWindow::~ConstructWindow()
{
    delete ui;
}

void ConstructWindow::switchDisplay(int) {
    const QString nationality
        = ui->shipnation->currentText().
                  localeAwareCompare(qtTrId("all-nationality")) == 0
              ? QLatin1String("") : ui->shipnation->currentText();
    const QString shiptype
        = ui->shiptype->currentText().
                  localeAwareCompare(qtTrId("all-shiptypes")) == 0
              ? QLatin1String("") : ui->shiptype->currentText();
    const QString shipclass
        = ui->shipclass->currentText().
                  localeAwareCompare(qtTrId("all-shipclasses")) == 0
              ? QLatin1String("") : ui->shipclass->currentText();
    const QString searchTerm = ui->searchBox->toPlainText();
    Client &engine = Client::getInstance();

    if(!engine.shipBPModel.isReady()) {
        QMessageBox msgBox(this);
        //% "Fetching ship blueprint data, please wait..."
        msgBox.setText(qtTrId("wait-for-blueprint"));
        engine.doRefreshFactoryAnchorage();
        QTimer timer;
        timer.setSingleShot(true);
        connect(&engine.shipBPModel, &ShipBPModel::bpReady,
                &msgBox, &QMessageBox::close);
        connect(&timer, &QTimer::timeout, &msgBox, &QMessageBox::close);
        timer.start(settings->value(
            "networkclient/downloadwaittimemsec", 80000).toInt());
        msgBox.exec();
    }
    if(!engine.equipModel.isReady()) {
        QMessageBox msgBox(this);
        //% "Fetching equipment data, please wait..."
        msgBox.setText(qtTrId("wait-for-equip"));
        engine.doRefreshFactoryArsenal();
        QTimer timer;
        timer.setSingleShot(true);
        connect(&engine.equipModel, &EquipModel::equipReady,
                &msgBox, &QMessageBox::close);
        connect(&timer, &QTimer::timeout, &msgBox, &QMessageBox::close);
        timer.start(settings->value(
            "networkclient/downloadwaittimemsec", 80000).toInt());
        msgBox.exec();
    }

    bool pass = true;
    bool pass1 = true;
    //% "All ship types"
    QStringList typePasses = {qtTrId("all-shiptypes")};
    //% "All ship classes"
    QStringList classPasses = {qtTrId("all-shipclasses")};;
    QList<int> namePasses = {};
    static auto meta = QMetaEnum::fromType<KP::AllegianceGroup>();
    for(auto iter = engine.shipBPModel.clientShipBPs.keyBegin();
         iter != engine.shipBPModel.clientShipBPs.keyEnd();
         ++iter) {
        Ship *ship = engine.shipRegistryCache[*iter];
        pass = true;

        if(!searchTerm.isEmpty()) {
            pass1 = false;
            if(QString::number(ship->getId()).contains(searchTerm)) {
                pass1 = true;
            }
            if(QString::number(ship->getId(), 16).contains(searchTerm)) {
                pass1 = true;
            }
            for(const auto &name:
                 std::as_const(ship->localNames)) {
                if(name.localeAwareCompare(searchTerm) == 0)
                    pass1 = true;
                if(name.contains(searchTerm, Qt::CaseInsensitive))
                    pass1 = true;
            }
            pass = pass1;
        }
        if(!nationality.isEmpty() &&
            qtTrId(meta.key(ship->getAllegianceGroup()))
                    .localeAwareCompare(nationality) != 0) {
            pass = false;
        }
        if(shiptype.isEmpty() && shipclass.isEmpty()) {
            if(pass) {
                QString type = ship->getType().toString();
                if(!typePasses.contains(type)
                    && type != qtTrId("all-shiptypes")) {
                    typePasses.append(type);
                }
            }
        }
        if(!shiptype.isEmpty() &&
            ship->getType().toString().localeAwareCompare(
                shiptype) != 0) {
            pass = false;
        }
        QString classText =
            ship->shipClassText[
                settings->value("client/language", "ja_JP").toString()
        ];
        if(classText.isEmpty()) {
            classText = ship->shipClassText["ja_JP"];
        }
        if(shipclass.isEmpty()) {
            if(pass) {
                if(!classPasses.contains(classText)
                    && classText != qtTrId("all-shipclasses")) {
                    classPasses.append(classText);
                }
            }
        }

        if(!shipclass.isEmpty() && classText.localeAwareCompare(
                                        shipclass) != 0) {
            pass = false;
        }

        if(pass && cloningMode
                && !ship->getPreviousModels(
                       engine.shipRegistryCache).isEmpty()) {
            pass = false;
        }
        if(pass) {
            namePasses.append(ship->getId());
        }
    }

    if(searchTerm.isEmpty() && shiptype.isEmpty() && shipclass.isEmpty()) {
        /* disconnect to eliminate infinite recursion */
        disconnect(ui->shiptype, &QComboBox::currentIndexChanged,
                   this, &ConstructWindow::switchDisplay);
        std::sort(typePasses.begin(), typePasses.end(), [](QString a, QString b)
                  { return a.localeAwareCompare(b) < 0; });
        ui->shiptype->clear();
        ui->shiptype->addItem(qtTrId("all-shiptypes"));
        ui->shiptype->addItems(typePasses);
        ui->shipclass->clear();
        update();
        connect(ui->shiptype, &QComboBox::currentIndexChanged,
                this, &ConstructWindow::switchDisplay);
    }
    else if(searchTerm.isEmpty() && shipclass.isEmpty()) {
        disconnect(ui->shipclass, &QComboBox::currentIndexChanged,
                   this, &ConstructWindow::switchDisplay);
        std::sort(classPasses.begin(), classPasses.end(),
                  [](QString a, QString b)
                  { return a.localeAwareCompare(b) < 0; });
        ui->shipclass->clear();
        ui->shipclass->addItem(qtTrId("all-shipclasses"));
        ui->shipclass->addItems(classPasses);
        update();
        connect(ui->shipclass, &QComboBox::currentIndexChanged,
                this, &ConstructWindow::switchDisplay);
    }

    if(ui->shipname->model() != engine.proxyModel) {
        engine.proxyModel->setSourceModel(&engine.shipDefModel);
        ui->shipname->setModel(engine.proxyModel);
    }
    engine.shipDefModel.setShips(namePasses);
    ui->shipname->model()->sort(0);
    shipNameChanged();

    update();
}

void ConstructWindow::initialize() {
    auto meta = QMetaEnum::fromType<KP::AllegianceGroup>();
    for(int i = 0; i < meta.keyCount(); ++i) {
        if(meta.value(i) == KP::AllegianceGroup::UnknownNation) {
            ui->shipnation->addItem(qtTrId("all-nationality"));
        }
        else {
            ui->shipnation->addItem(qtTrId(meta.key(i)));
        }
    }
}

void ConstructWindow::shipNameChanged(int) {
    Client &engine = Client::getInstance();
    auto index = engine.proxyModel->mapToSource(
        engine.proxyModel->index(ui->shipname->currentIndex(), 0));
    if(!index.isValid()) {
        for(int i = 0; i < KP::maxEquipSlots; ++i) {
            engine.specModels[i]->setEquip(0);
        }
        engine.shipRemodelModel->setShip(QList<int>());
        shipDef = 0;
        return;
    }
    auto *ship = engine.shipDefModel.getCurrentShip(index);
    if(ship == nullptr) {
        shipDef = 0;
    }
    else {
        shipDef = ship->getId();
    }
    for(int i = 0; i < KP::maxEquipSlots; ++i) {
        char str[80];
        strcpy(str, "Defaultequip");
        auto equip = ship->attr[
            strcat(str, QString::number(i+1).toLatin1().constData())];
        engine.specModels[i]->setEquip(equip);
        auto *box = equipBoxes[i];
        auto *label = ui->gridLayout->itemAtPosition(i, 2)->widget();
        auto *equipText = qobject_cast<QLabel *>(
            ui->gridLayout->itemAtPosition(i, 3)->widget());

        if(equipText) {
            if(equip != 0) {
                equipText->setText(
                    engine.equipRegistryCache[equip]->toString());
            }
            else {
                //% "(Not required)"
                equipText->setText(qtTrId("default-equip-none"));
            }
        }
        if(box->count() > 0) {
            box->setCurrentIndex(0);
            if(label) {
                label->setStyleSheet("");
            }
        }
        else if(equip != 0) {
            if(label) {
                label->setStyleSheet("color: red;");
            }
        }
        else {
            if(label) {
                label->setStyleSheet("");
            }
        }
    }
    ui->resourceAmount->setText(ship->consRes().toString(true));

shipToRemodel:
    if(ui->shipnameToRemodel->model() != engine.shipRemodelModel) {
        ui->shipnameToRemodel->setModel(engine.shipRemodelModel);
    }
    QList<int> availableShipsToRemodel;
    auto allships = engine.shipRegistryCache;
    for(auto shipattr = allships.keyValueBegin();
         shipattr != allships.keyValueEnd();
         ++shipattr) {
        if(shipattr->second->attr["remodel"] == ship->getId()) {
            availableShipsToRemodel.append(shipattr->first);
        }
    }
    engine.shipRemodelModel->setShip(availableShipsToRemodel);
    if(ui->shipnameToRemodel->count() > 0
        || ship->getPreviousModels(engine.shipRegistryCache).isEmpty()) {
        ui->remodelLabel->setStyleSheet("");
    }
    else {
        ui->remodelLabel->setStyleSheet("color: red;");
    }
    updateSanityDisplay();

check_cloning_allowed:
    if(!cloningMode) {
        bool canProceed = true;
        if(shipDef != 0 && ship != nullptr) {
            auto latermodels =
                ship->getLaterModels(engine.shipRegistryCache);
            int latestmodel = shipDef;
            if(!latermodels.empty()) {
                latestmodel = *std::max_element(
                    latermodels.constBegin(),
                    latermodels.constEnd());
            }
            QList<int> groupDefs;
            for(auto iter =
                     engine.shipRegistryCache.keyValueBegin();
                 iter !=
                     engine.shipRegistryCache.keyValueEnd();
                 ++iter) {
                auto lm = iter->second->getLaterModels(
                    engine.shipRegistryCache);
                int lmId = lm.empty() ? iter->first
                    : *std::max_element(
                          lm.constBegin(), lm.constEnd());
                if(lmId == latestmodel) {
                    groupDefs.append(iter->first);
                }
            }
            auto ownedShips = engine.shipModel.getAllShips();
            int count = 0;
            for(auto s: std::as_const(ownedShips)) {
                if(groupDefs.contains(s->getId())) { ++count; }
            }
            canProceed = (count == 0)
                || (ui->shipnameToRemodel->count() > 0);
        }
        ui->buttonBox->button(QDialogButtonBox::Ok)
            ->setEnabled(canProceed);
    }
}

int ConstructWindow::shipDefDesired() {
    return shipDef;
}

QList<QUuid> ConstructWindow::defaultEquipsDesired() {
    QList<QUuid> result(KP::maxEquipSlots, QUuid());
    Client &engine = Client::getInstance();
    for(int i = 0; i < KP::maxEquipSlots; ++i) {
        SpecEquipModel *model = engine.specModels[i];
        auto uid = model->data(
            model->index(equipBoxes[i]->currentIndex(), 0), Qt::ToolTipRole);
        result[i] = uid.isValid() ? uid.toUuid() : QUuid();
    }
    return result;
}

QUuid ConstructWindow::shipToRemodelDesired() {
    Client &engine = Client::getInstance();
    SpecShipModel *model = engine.shipRemodelModel;
    auto uid = model->data(
        model->index(ui->shipnameToRemodel->currentIndex(), 0),
        Qt::ToolTipRole);
    return uid.isValid() ? uid.toUuid() : QUuid();
}

void ConstructWindow::setCloningMode(bool cloning) {
    cloningMode = cloning;
    if(cloning) {
        //% "Cloning Vats"
        setWindowTitle(qtTrId("cloning-vats"));
    }
    else {
        //% "Construct Ships"
        setWindowTitle(qtTrId("construct-ships"));
    }
    ui->sanityFrame->setVisible(cloning);
    ui->remodelLabel->setVisible(!cloning);
    ui->shipnameToRemodel->setVisible(!cloning);
    updateSanityDisplay();
}

void ConstructWindow::updateSanityDisplay() {
    if(!cloningMode) {
        return;
    }
    Client &engine = Client::getInstance();
    double remaining = engine.exoticCache.sanity;
    double required = 0.0;
    if(shipDef != 0) {
        Ship *ship = engine.shipRegistryCache.value(shipDef);
        if(ship) {
            auto latermodels = ship->getLaterModels(
                engine.shipRegistryCache);
            int latestmodel = shipDef;
            if(!latermodels.empty()) {
                latestmodel = *std::max_element(
                    latermodels.constBegin(),
                    latermodels.constEnd());
            }
            QList<int> groupDefs;
            for(auto iter =
                     engine.shipRegistryCache.keyValueBegin();
                 iter !=
                     engine.shipRegistryCache.keyValueEnd();
                 ++iter) {
                auto lm = iter->second->getLaterModels(
                    engine.shipRegistryCache);
                int lmId = lm.empty() ? iter->first
                    : *std::max_element(
                          lm.constBegin(), lm.constEnd());
                if(lmId == latestmodel) {
                    groupDefs.append(iter->first);
                }
            }
            auto ownedShips = engine.shipModel.getAllShips();
            int count = 0;
            for(auto s: std::as_const(ownedShips)) {
                if(groupDefs.contains(s->getId())) {
                    ++count;
                }
            }
            if(count > 0) {
                required = std::pow(2.0, count - 1);
            }
        }
    }
    ui->sanityRemainingValue->setText(
        QString::number(remaining, 'f', 2));
    ui->sanityRequiredValue->setText(
        QString::number(required, 'f', 2));
}
