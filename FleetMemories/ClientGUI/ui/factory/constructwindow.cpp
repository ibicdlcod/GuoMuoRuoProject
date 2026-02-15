#include "constructwindow.h"
#include "../../clientv2.h"
#include "ui_constructwindow.h"
#include "../../../Protocol/kp.h"
#include <QMetaEnum>
#include <QMessageBox>
#include <QUuid>

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
    Clientv2 &engine = Clientv2::getInstance();
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
    Clientv2 &engine = Clientv2::getInstance();

    if(!engine.shipBPModel.isReady()) {
        QMessageBox msgBox(this);
        //% "Fetching ship blueprint data, please wait..."
        msgBox.setText(qtTrId("wait-for-blueprint"));
        engine.doRefreshFactoryAnchorage();
        QTimer timer;
        timer.setSingleShot(true);
        connect(&engine.shipBPModel, &ShipBPModel::bpReady, &msgBox, &QMessageBox::close);
        connect(&timer, &QTimer::timeout, &msgBox, &QMessageBox::close);
        timer.start(settings->value("networkclient/downloadwaittimemsec", 80000).toInt());
        msgBox.exec();
    }
    if(!engine.equipModel.isReady()) {
        QMessageBox msgBox(this);
        //% "Fetching equipment data, please wait..."
        msgBox.setText(qtTrId("wait-for-equip"));
        engine.doRefreshFactoryArsenal();
        QTimer timer;
        timer.setSingleShot(true);
        connect(&engine.equipModel, &EquipModel::equipReady, &msgBox, &QMessageBox::close);
        connect(&timer, &QTimer::timeout, &msgBox, &QMessageBox::close);
        timer.start(settings->value("networkclient/downloadwaittimemsec", 80000).toInt());
        msgBox.exec();
    }

    bool pass = true;
    bool pass1 = true;
    //% "All ship types"
    QStringList typePasses = {qtTrId("all-shiptypes")};
    //% "All ship classes"
    QStringList classPasses = {qtTrId("all-shipclasses")};;
    QList<int> namePasses = {};
    static auto meta = QMetaEnum::fromType<KP::ShipNationality>();
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
            qtTrId(meta.key(ship->getNationality()))
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
        std::sort(classPasses.begin(), classPasses.end(), [](QString a, QString b)
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

void ConstructWindow::shipNameChanged(int) {
    Clientv2 &engine = Clientv2::getInstance();
    auto index = engine.proxyModel->mapToSource(engine.proxyModel->index(ui->shipname->currentIndex(), 0));
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
        engine.specModels[i]->setEquip(ship->attr[
            strcat(str, QString::number(i+1).toLatin1().constData())]);
    }
    for(auto *box: equipBoxes) {
        if(box->count() > 0) {
            box->setCurrentIndex(0);
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
}

int ConstructWindow::shipDefDesired() {
    return shipDef;
}

QList<QUuid> ConstructWindow::defaultEquipsDesired() {
    QList<QUuid> result(KP::maxEquipSlots, QUuid());
    Clientv2 &engine = Clientv2::getInstance();
    for(int i = 0; i < KP::maxEquipSlots; ++i) {
        SpecEquipModel *model = engine.specModels[i];
        auto uid = model->data(
            model->index(equipBoxes[i]->currentIndex(), 0), Qt::ToolTipRole);
        result[i] = uid.isValid() ? uid.toUuid() : QUuid();
    }
    return result;
}

QUuid ConstructWindow::shipToRemodelDesired() {
    Clientv2 &engine = Clientv2::getInstance();
    SpecShipModel *model = engine.shipRemodelModel;
    auto uid = model->data(
        model->index(ui->shipnameToRemodel->currentIndex(), 0), Qt::ToolTipRole);
    return uid.isValid() ? uid.toUuid() : QUuid();
}
