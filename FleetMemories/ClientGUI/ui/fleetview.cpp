#include "fleetview.h"
#include "ui_fleetview.h"
#include <QScrollArea>
#include <QMetaEnum>
#include <QStyleHints>
#include "interactivelabel.h"
#include "../clientv2.h"
#include "../../Protocol/kp.h"

extern std::unique_ptr<QSettings> settings;

FleetView::FleetView(QWidget *parent)
    : QFrame(parent)
    , ui(new Ui::FleetView), equipView(nullptr)
{
    ui->setupUi(this);
    equipView.hide();

    Clientv2 &engine = Clientv2::getInstance();
    connect(&engine, &Clientv2::receivedAnchorageShip,
            this, &FleetView::receivedShipInfo);
    QWidget *fleetGrid = new QWidget(this);

    grid = new QGridLayout(fleetGrid);
    grid->setContentsMargins(3, 3, 3, 3);
    grid->setSpacing(3);
    grid->setHorizontalSpacing(10);
    QLabel *fleetPosHeader = new QLabel(this);
    fleetPosHeader->setObjectName(QStringLiteral("fleetPos-Head"));
    fleetPosHeader->setAlignment(Qt::AlignCenter);
    grid->addWidget(fleetPosHeader, 0, posColumn);
    //% "Pos"
    fleetPosHeader->setText(qtTrId("fleet-pos-head"));
    for(int i = 0; i < KP::combinedFleetSize; ++i) {
        QLabel *fleetPos = new QLabel(this);
        fleetPos->setObjectName(QString("fleetPos-%1").arg(i+1));
        fleetPos->setAlignment(Qt::AlignCenter);
        grid->addWidget(fleetPos, i+1, posColumn);
        fleetPos->setText(QString("%1").arg(i+1));
        auto font = fleetPos->font();
        font.setPointSize(30);
        fleetPos->setFont(font);
    }
    QLabel *shipNameHeader = new QLabel(this);
    shipNameHeader->setObjectName(QStringLiteral("shipName-Head"));
    shipNameHeader->setAlignment(Qt::AlignCenter);
    grid->addWidget(shipNameHeader, 0, nameColumn);
    //% ""
    //shipNameHeader->setText(qtTrId("ship-name-head"));
    for(int i = 0; i < KP::combinedFleetSize; ++i) {
        QLabel *shipName = new QLabel(this);
        shipName->setObjectName(QString("shipName-%1").arg(i+1));
        shipName->setAlignment(Qt::AlignCenter);
        auto font = shipName->font();
        shipName->setMaximumSize(QSize(200, 55));
        font.setPointSize(40);
        shipName->setFont(font);
        grid->addWidget(shipName, i+1, nameColumn);
        //% ""
        //shipName->setText(qtTrId("fleet-no-ship"));
    }
    for(int i = 0; i < KP::combinedFleetSize; ++i) {
        InteractiveLabel *fleetIcon = new InteractiveLabel(i, this);
        fleetIcon->setObjectName(QString("fleetIcon-%1").arg(i+1));
        fleetIcon->setAlignment(Qt::AlignCenter);
        fleetIcon->setMinimumSize(QSize(60, 60));
        grid->addWidget(fleetIcon, i+1, shipIconColumn);
    }

    QVBoxLayout *greatLayout = new QVBoxLayout(this);
    greatLayout->setContentsMargins(3, 3, 3, 3);
    greatLayout->setSpacing(3);
    greatLayout->addWidget(ui->FleetMenu);
    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setStyleSheet(
        "QScrollArea { border-style: none; }");
    scrollArea->setWidget(fleetGrid);
    scrollArea->setAlignment(Qt::AlignHCenter);
    greatLayout->addWidget(scrollArea);

    auto meta = QMetaEnum::fromType<KP::FleetType>();
    for(int i = 0; i < meta.keyCount(); ++i) {
        ui->fleetTypeSelect->addItem(qtTrId(meta.key(i)));
    }
    connect(ui->fleetTypeSelect, &QComboBox::activated,
            this, &FleetView::modifyFleetType);
    for(int i = KP::normalFleetSize; i < KP::combinedFleetSize; ++i) {
        for(int j = 0; j < grid->columnCount(); ++j) {
            grid->itemAtPosition(i + 1, j)->widget()->hide();
        }
    }
    grid->setSizeConstraint(QLayout::SetFixedSize);
    connect(ui->fleet_1, &QPushButton::clicked,
            this, &FleetView::modifyFleetIndex);
    connect(ui->fleet_2, &QPushButton::clicked,
            this, &FleetView::modifyFleetIndex);
    connect(ui->fleet_3, &QPushButton::clicked,
            this, &FleetView::modifyFleetIndex);
    connect(ui->fleet_4, &QPushButton::clicked,
            this, &FleetView::modifyFleetIndex);
    ui->fleet_1->setEnabled(false);
    QList<QPushButton *> fleetButtons{ui->fleet_1, ui->fleet_2,
                                      ui->fleet_3, ui->fleet_4};
    assert(fleetButtons.size() == KP::fleetsSize);
    for(auto fleetButton: fleetButtons) {
        switch(QApplication::styleHints()->colorScheme()) {
        case Qt::ColorScheme::Dark:
            fleetButton->setStyleSheet("QPushButton:disabled {"
                                       "background-color: #8080FF;"
                                       "color: #000000;"
                                       "}");
            break;
        case Qt::ColorScheme::Light: [[fallthrough]];
        default:
            fleetButton->setStyleSheet("QPushButton:disabled {"
                                       "background-color: #000080;"
                                       "color: #FFFFFF;"
                                       "}");
            break;
        }
    }
    connect(ui->saveButton, &QPushButton::clicked,
            this, &FleetView::sendFleetData);
}

FleetView::~FleetView()
{
    delete ui;
}

void FleetView::modifyFleetIndex(bool checked) {
    Q_UNUSED(checked)
    QString sender = QObject::sender()->objectName();
    int newFleetIndex = 0;
    if(sender.startsWith("fleet_")) {
        newFleetIndex = sender.last(sender.size()
                                    - QStringLiteral("fleet_").size())
                            .toInt() - 1;
    }
    if(newFleetIndex == currentActiveFleet) {
        return;
    }
    else {
        currentActiveFleet = newFleetIndex;
        auto newType = fleetTypes[currentActiveFleet];
        ui->fleetTypeSelect->setCurrentIndex(newType);
        ui->fleet_1->setEnabled(true);
        ui->fleet_2->setEnabled(true);
        ui->fleet_3->setEnabled(true);
        ui->fleet_4->setEnabled(true);
        qobject_cast<QPushButton *>(QObject::sender())->setEnabled(false);
        if(newType == KP::NormalFleet) {
            for(int i = KP::normalFleetSize; i < KP::combinedFleetSize; ++i) {
                modifyFleetShip(i, QUuid());
                for(int j = 0; j < grid->columnCount(); ++j) {
                    grid->itemAtPosition(i + 1, j)->widget()->hide();
                }
            }
            for(int i = 0; i < KP::normalFleetSize; ++i) {
                modifyFleetShip(i, ships[FleetPos{currentActiveFleet, i}]);
            }
        }
        else {
            for(int i = KP::normalFleetSize; i < KP::combinedFleetSize; ++i) {
                for(int j = 0; j < grid->columnCount(); ++j) {
                    grid->itemAtPosition(i + 1, j)->widget()->show();
                }
            }
            for(int i = 0; i < KP::combinedFleetSize; ++i) {
                modifyFleetShip(i, ships[FleetPos{currentActiveFleet, i}]);
            }
        }
    }
}

void FleetView::modifyFleetShip(int posIndex, QUuid uid) {
    Clientv2 &engine = Clientv2::getInstance();
    auto shipModel = &engine.shipModel;
    FleetPos oldPos = FleetPos({-1, -1});
    FleetPos newPos = FleetPos({currentActiveFleet, posIndex});
    for(auto iter = ships.keyValueBegin();
         iter != ships.keyValueEnd();
         ++iter) {
        if(iter->second == uid && !uid.isNull()) {
            oldPos = iter->first;
        }
    }
    if(oldPos != FleetPos({-1, -1}) && oldPos != newPos) {
        /* do switch */
        auto oldUid = ships.contains(newPos)
                          ? ships[newPos] : QUuid();
        ships[oldPos] = oldUid;
        if(!oldUid.isNull()) {
            std::get<1>(shipModel->getShip(oldUid))->fleetIndex
                = oldPos.fleetindex;
            std::get<1>(shipModel->getShip(oldUid))->fleetPosIndex
                = oldPos.posindex;
        }
        if(oldPos.fleetindex == currentActiveFleet) {
            qobject_cast<InteractiveLabel *>
                (grid->itemAtPosition(oldPos.posindex + 1, 2)->widget())
                    ->shipSelected(oldUid);
            auto oldText = qobject_cast<QLabel *>
                (grid->itemAtPosition(oldPos.posindex + 1, 1)->widget());
            if(!oldUid.isNull()) {
                QString oldName
                    = std::get<0>(shipModel->getShip(oldUid))->toString(
                        settings->value("client/language", "ja_JP").toString());
                if(oldName.isEmpty()) {
                    oldName = std::get<0>(shipModel->getShip(oldUid))
                                  ->toString("ja_JP");
                }
                oldText->setText(oldName);
            }
            else {
                oldText->setText("");
            }
        }
    }
    if(uid.isNull() && !ships[newPos].isNull()) {
        std::get<1>(shipModel->getShip(ships[newPos]))->fleetIndex
            = -1;
        std::get<1>(shipModel->getShip(ships[newPos]))->fleetPosIndex
            = -1;
    }
    ships[newPos] = uid;
    qobject_cast<InteractiveLabel *>
        (grid->itemAtPosition(newPos.posindex + 1, 2)->widget())
            ->updateShipUId(uid);
    auto newText = qobject_cast<QLabel *>
        (grid->itemAtPosition(newPos.posindex + 1, 1)->widget());
    if(!uid.isNull()) {
        std::get<1>(shipModel->getShip(uid))->fleetIndex
            = newPos.fleetindex;
        std::get<1>(shipModel->getShip(uid))->fleetPosIndex
            = newPos.posindex;
        QString newName = std::get<0>(shipModel->getShip(uid))
                              ->toString(settings
                                             ->value("client/language", "ja_JP")
                                             .toString());
        if(newName.isEmpty()) {
            newName = std::get<0>(shipModel->getShip(uid))->toString("ja_JP");
        }
        newText->setText(newName);
        QFont font = newText->font();
        int fontSize = 1;
        while(true)
        {
            font.setPointSize(fontSize);
            QRect r = QFontMetrics(font).boundingRect(newText->text());
            if (r.height() < newText->maximumHeight()
                && r.width() < newText->maximumWidth())
                fontSize++;
            else {
                fontSize--;
                font.setPointSize(fontSize);
                break;
            }
        }
        newText->setFont(font);
    }
    else {
        newText->setText("");
    }
}

void FleetView::modifyFleetType(int index) {
    auto meta = QMetaEnum::fromType<KP::FleetType>();
    auto newType = static_cast<KP::FleetType>(meta.value(index));
    fleetTypes[currentActiveFleet] = newType;
    if(newType == KP::NormalFleet) {
        for(int i = KP::normalFleetSize; i < KP::combinedFleetSize; ++i) {
            ships.remove(FleetPos{currentActiveFleet, i});
            modifyFleetShip(i, QUuid());
            for(int j = 0; j < grid->columnCount(); ++j) {
                grid->itemAtPosition(i + 1, j)->widget()->hide();
            }
        }
    }
    else {
        for(int i = KP::normalFleetSize; i < KP::combinedFleetSize; ++i) {
            for(int j = 0; j < grid->columnCount(); ++j) {
                grid->itemAtPosition(i + 1, j)->widget()->show();
            }
        }
    }
}

void FleetView::receivedShipInfo(const QJsonObject &info) {
    ships.clear();
    Clientv2 &engine = Clientv2::getInstance();
    QJsonArray inputArray = info["content"].toArray();
    for(const QJsonValueRef item: inputArray) {
        QJsonObject itemObject = item.toObject();
        QUuid uid = QUuid(itemObject["serial"].toString());
        /*Ship *ship = engine.getShipReg(
            itemObject["def"].toInt());*/
        ShipDynamic *attr = new ShipDynamic(itemObject);
        FleetPos pos{attr->fleetIndex, attr->fleetPosIndex};
        if(pos != FleetPos{-1, -1}) {
            ships[pos] = uid;
            fleetTypes[attr->fleetIndex] =
                static_cast<KP::FleetType>(itemObject["fleettype"].toInt());
        }
        delete attr;
    }
    ui->fleetTypeSelect->setCurrentIndex(fleetTypes[currentActiveFleet]);
    if(fleetTypes[currentActiveFleet] == KP::NormalFleet) {
        for(int i = KP::normalFleetSize; i < KP::combinedFleetSize; ++i) {
            modifyFleetShip(i, QUuid());
            for(int j = 0; j < grid->columnCount(); ++j) {
                grid->itemAtPosition(i + 1, j)->widget()->hide();
            }
        }
        for(int i = 0; i < KP::normalFleetSize; ++i) {
            modifyFleetShip(i, ships[FleetPos{currentActiveFleet, i}]);
        }
    }
    else {
        for(int i = KP::normalFleetSize; i < KP::combinedFleetSize; ++i) {
            for(int j = 0; j < grid->columnCount(); ++j) {
                grid->itemAtPosition(i + 1, j)->widget()->show();
            }
        }
        for(int i = 0; i < KP::combinedFleetSize; ++i) {
            modifyFleetShip(i, ships[FleetPos{currentActiveFleet, i}]);
        }
    }
}

void FleetView::sendFleetData(bool checked) {
    Q_UNUSED(checked)
    QJsonArray content;
    for(auto iter = ships.constKeyValueBegin();
         iter != ships.constKeyValueEnd();
         ++iter) {
        if(iter->second.isNull()) {
            continue;
        }
        QJsonObject ship;
        ship["uuid"] = iter->second.toString();
        ship["pos"] =
            iter->first.fleetindex * FleetPos::fleetRep
            + iter->first.posindex;
        ship["fleettype"] = fleetTypes[iter->first.fleetindex];
        content.append(ship);
    }
    Clientv2 &engine = Clientv2::getInstance();
    engine.sendFleetData(content);
}
