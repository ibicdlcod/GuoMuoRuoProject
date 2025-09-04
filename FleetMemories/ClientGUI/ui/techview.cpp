#include "techview.h"
#include "ui_techview.h"
#include "../clientv2.h"
#include "../networkerror.h"
#include "../equipicon.h"

extern std::unique_ptr<QSettings> settings;

TechView::TechView(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::TechView)
{
    ui->setupUi(this);

    Clientv2 &engine = Clientv2::getInstance();
    connect(&engine, &Clientv2::receivedGlobalTechInfo,
            this, &TechView::updateGlobalTech);
    connect(&engine, &Clientv2::receivedGlobalTechInfo2,
            this, &TechView::updateGlobalTechViewTable);
    connect(&engine, &Clientv2::receivedLocalTechInfo,
            this, &TechView::updateLocalTech);
    connect(&engine, &Clientv2::receivedLocalTechInfo2,
            this, &TechView::updateLocalTechViewTable);
    connect(&engine, &Clientv2::equipRegistryComplete,
            this->ui->waitText, &QLabel::hide);
    connect(&engine, &Clientv2::equipRegistryComplete,
            this, &TechView::resetLocalListName);
    connect(this->ui->updateGlobalButton, &QPushButton::clicked,
            this, &TechView::demandGlobalTech);
    connect(&engine, &Clientv2::receivedSkillPointInfo,
            this, &TechView::updateSkillPoints);
    ui->globalViewTable->hide();
    ui->waitText->show();
    ui->waitText->setWordWrap(true);
    ui->waitText->setText(
        QStringLiteral("updating equipment data, please wait..."));
    QHeaderView *horizationalH = ui->globalViewTable->horizontalHeader();
    horizationalH->setSectionResizeMode(QHeaderView::ResizeToContents);
    QHeaderView *verticalH = ui->globalViewTable->verticalHeader();
    verticalH->sectionResizeMode(QHeaderView::ResizeToContents);

    QList<QString> sortedGroups = EquipType::getDisplayGroupsSorted();

    for(auto &equipType: sortedGroups) {
        if(equipType.compare("VIRTUAL", Qt::CaseInsensitive) == 0)
            continue;
        ui->localListType1->addItem(equipType);
    }
    //% "All equipments"
    ui->localListType1->addItem(qtTrId("all-equipments"));

    auto meta = QMetaEnum::fromType<KP::ShipNationality>();
    for(int i = 0; i < meta.keyCount(); ++i) {
        if(meta.value(i) == KP::ShipNationality::Unknown) {
            //% "All nationalities"
            ui->localListNation->addItem(qtTrId("all-nationality"));
        }
        else {
            ui->localListNation->addItem(qtTrId(meta.key(i)));
        }
    }

    ui->localListType1->setCurrentIndex(0);
    connect(ui->localListType1, &QComboBox::activated,
            this, &TechView::resetLocalListName);
    connect(ui->localListEquip, &QComboBox::activated,
            this, &TechView::demandLocalTech);
    connect(ui->localListEquip, &QComboBox::activated,
            this, &TechView::demandSkillPoints);
    connect(ui->localListNation, &QComboBox::activated,
            this, &TechView::resetLocalListName);
    connect(ui->localListType2, &QComboBox::activated,
            this, &TechView::resetLocalListName);
    connect(ui->localListClass, &QComboBox::activated,
            this, &TechView::resetLocalListName);
    connect(ui->localListShip, &QComboBox::activated,
            this, &TechView::demandLocalTech);
    connect(ui->updateLocal, &QPushButton::clicked,
            this, &TechView::demandLocalTech);
    connect(ui->updateLocal, &QPushButton::clicked,
            this, &TechView::demandSkillPoints);

    ui->globalViewTable->setSortingEnabled(true);
    ui->localViewTable->setSortingEnabled(true);
    ui->shipChoice->hide();
    connect(ui->equipOrShip, &QPushButton::clicked,
            this, &TechView::equipOrShip);
}

TechView::~TechView()
{
    delete ui;
}

void TechView::demandGlobalTech() {
    Clientv2 &engine = Clientv2::getInstance();
    if(!engine.isEquipRegistryCacheGood())
        return;
    else {
        engine.switchToTech2();
    }
}

void TechView::demandLocalTech(int index) {
    Q_UNUSED(index)

    ui->localViewTable->clear();
    ui->skillPointsValue->setText(qtTrId("techview-na"));

    Clientv2 &engine = Clientv2::getInstance();
    if(isEquipChoice) {
        for(auto &equipReg:
             engine.equipRegistryCache) {
            for(auto &name: equipReg->localNames) {
                if(name.localeAwareCompare(ui->localListEquip->currentText())
                    == 0) {
                    engine.socket.flush();
                    QByteArray msg = KP::clientDemandTech(equipReg->getId());
                    const qint64 written = engine.socket.write(msg);
                    if (written <= 0) {
                        throw NetworkError(engine.socket.errorString());
                    }
                    return;
                }
            }
        }
    }
    else {
        for(auto &shipReg:
             engine.shipRegistryCache) {
            for(auto &name: shipReg->localNames) {
                if(name.localeAwareCompare(ui->localListShip->currentText())
                    == 0) {
                    engine.socket.flush();
                    QByteArray msg = KP::clientDemandTech(shipReg->getId());
                    const qint64 written = engine.socket.write(msg);
                    if (written <= 0) {
                        throw NetworkError(engine.socket.errorString());
                    }
                    return;
                }
            }
        }
    }
}

void TechView::demandSkillPoints(int index) {
    Q_UNUSED(index)

    if(isEquipChoice) {
        Clientv2 &engine = Clientv2::getInstance();
        for(auto &equipReg:
             engine.equipRegistryCache) {
            for(auto &name: equipReg->localNames) {
                if(name.compare(ui->localListEquip->currentText(),
                                 Qt::CaseInsensitive) == 0) {
                    engine.socket.flush();
                    QByteArray msg = KP::clientDemandSkillPoints(equipReg->getId());
                    const qint64 written = engine.socket.write(msg);
                    if (written <= 0) {
                        throw NetworkError(engine.socket.errorString());
                    }
                    return;
                }
            }
        }
    }
}

void TechView::equipOrShip() {
    if(isEquipChoice) {
        isEquipChoice = false;
        //% "Switch to Equip"
        ui->equipOrShip->setText(qtTrId("techview-toequip"));
        ui->equipChoice->hide();
        ui->shipChoice->show();

        resetLocalListName();
    }
    else {
        isEquipChoice = true;
        ui->equipOrShip->setText(qtTrId("techview-toship"));
        ui->equipChoice->show();
        ui->shipChoice->hide();
    }
}

void TechView::updateGlobalTech(const QJsonObject &djson) {
    ui->globalTechValue->setText(
        QString::number(djson.value("value").toDouble()));
    Clientv2 &engine = Clientv2::getInstance();
    engine.techCache[0] = djson.value("value").toDouble();
}

void TechView::updateGlobalTechViewTable(const QJsonObject &djson) {
    Clientv2 &engine = Clientv2::getInstance();

    if(!engine.isEquipRegistryCacheGood()) {
        ui->globalViewTable->hide();
        ui->waitText->show();
        ui->waitText->setWordWrap(true);
        ui->waitText->setText(
            QStringLiteral("updating equipment data, please wait..."));
        engine.demandEquipCache();
        return;
    }
    else if(!engine.isShipRegistryCacheGood()) {
        ui->globalViewTable->hide();
        ui->waitText->show();
        ui->waitText->setWordWrap(true);
        ui->waitText->setText(
            QStringLiteral("updating ship data, please wait..."));
        engine.demandShipCache();
        return;
    }
    else {
        ui->globalViewTable->setSortingEnabled(false);
        ui->globalViewTable->show();
        ui->waitText->hide();
    }
    ui->globalViewTable->setColumnCount(5);
    QJsonArray contents = djson["content"].toArray();
    int currentRowCount = ui->globalViewTable->rowCount();
    ui->globalViewTable->clear();
    currentRowCount = 0;
    ui->globalViewTable->setRowCount(contents.size());
    int i = 0;
    for(auto &content: std::as_const(contents)) {
        QJsonObject item = content.toObject();
        QTableWidgetItem *newItem = new QTableWidgetItem(
            item["serial"].toString().first(9).last(8));
        newItem->setFlags(newItem->flags() & ~Qt::ItemIsEditable);
        newItem->setToolTip(item["serial"].toString());
        ui->globalViewTable->setItem(currentRowCount + i, 0, newItem);

        Equipment *thisEquip = nullptr;
        Ship *thisShip = nullptr;
        if(item["def"].toInt() < KP::equipIdMax) {
            thisEquip = engine.getEquipmentReg(item["def"].toInt());
        }
        else {
            thisShip = engine.getShipReg(item["def"].toInt());
        }
        if(thisEquip && !thisEquip->isInvalid()) {
            QTableWidgetItem *newItem2;
            newItem2 = new QTableWidgetItem(
                thisEquip->toString(settings->value("language", "ja_JP").toString()));
            newItem2->setIcon(Icute::equipIcon(thisEquip->type, false));
            newItem2->setFlags(newItem->flags() & ~Qt::ItemIsEditable);
            ui->globalViewTable->setItem(currentRowCount + i, 1, newItem2);
            QTableWidgetItem *newItem3 = new TableWidgetItemNumber(
                thisEquip->getTech());
            newItem3->setFlags(newItem->flags() & ~Qt::ItemIsEditable);
            ui->globalViewTable->setItem(currentRowCount + i, 2, newItem3);
            QTableWidgetItem *newItem4 = new TableWidgetItemNumber(
                item["weight"].toDouble());
            newItem4->setFlags(newItem->flags() & ~Qt::ItemIsEditable);
            ui->globalViewTable->setItem(currentRowCount + i, 3, newItem4);
            QTableWidgetItem *newItem5 = new TableWidgetItemNumber(
                thisEquip->type.getTypeSort());
            newItem5->setFlags(newItem->flags() & ~Qt::ItemIsEditable);
            ui->globalViewTable->setItem(currentRowCount + i, 4, newItem5);
        }
        if(thisShip) {
            QTableWidgetItem *newItem2;
            newItem2 = new QTableWidgetItem(
                thisShip->toString(settings->value("language", "ja_JP").toString()));
            newItem2->setIcon(Icute::shipIcon(thisShip->getId(), false));
            newItem2->setFlags(newItem->flags() & ~Qt::ItemIsEditable);
            ui->globalViewTable->setItem(currentRowCount + i, 1, newItem2);
            QTableWidgetItem *newItem3 = new TableWidgetItemNumber(
                thisShip->getTech());
            newItem3->setFlags(newItem->flags() & ~Qt::ItemIsEditable);
            ui->globalViewTable->setItem(currentRowCount + i, 2, newItem3);
            QTableWidgetItem *newItem4 = new TableWidgetItemNumber(
                item["weight"].toDouble());
            newItem4->setFlags(newItem->flags() & ~Qt::ItemIsEditable);
            ui->globalViewTable->setItem(currentRowCount + i, 3, newItem4);
            QTableWidgetItem *newItem5 = new TableWidgetItemNumber(
                -(thisShip->getId()));
            newItem5->setFlags(newItem->flags() & ~Qt::ItemIsEditable);
            ui->globalViewTable->setItem(currentRowCount + i, 4, newItem5);
        }
        ++i;
    }
    ui->globalViewTable->setHorizontalHeaderLabels(
        {
            //% "Serial Num"
            qtTrId("Serial-num"),
            //% "Name"
            qtTrId("Equip-name-def"),
            //% "Tech"
            qtTrId("Equip-tech-level"),
            //% "Weight"
            qtTrId("Weight")
        });
    ui->globalViewTable->setSortingEnabled(true);
    ui->globalViewTable->sortByColumn(4, Qt::DescendingOrder);
    ui->globalViewTable->sortByColumn(2, Qt::DescendingOrder);
    ui->globalViewTable->hideColumn(4);
    QTimer::singleShot(1, this, [this]{resizeColumns(true);});
}

void TechView::updateLocalTech(const QJsonObject &djson) {
    ui->localTechValue->setText(
        QString::number(djson.value("value").toDouble()));
    Clientv2 &engine = Clientv2::getInstance();
    engine.techCache[djson["jobid"].toInt()]
        = djson.value("value").toDouble();
}

void TechView::updateLocalTechViewTable(const QJsonObject &djson) {
    Clientv2 &engine = Clientv2::getInstance();

    if(isEquipChoice && !engine.isEquipRegistryCacheGood()) {
        return;
    }
    else if(!isEquipChoice && !engine.isShipRegistryCacheGood()) {
        return;
    }
    else {
        ui->localViewTable->setSortingEnabled(false);
        ui->localViewTable->show();
    }
    ui->localViewTable->setHorizontalHeaderLabels(
        {qtTrId("Serial-num"), qtTrId("Equip-name-def"),
         qtTrId("Equip-tech-level"), qtTrId("Weight")});
    ui->localViewTable->setColumnCount(5);
    QJsonArray contents = djson["content"].toArray();
    int currentRowCount = ui->localViewTable->rowCount();
    ui->localViewTable->clear();
    currentRowCount = 0;
    ui->localViewTable->setRowCount(contents.size());
    int i = 0;
    for(auto &content: std::as_const(contents)) {
        QJsonObject item = content.toObject();
        QTableWidgetItem *newItem = new QTableWidgetItem(
            item["serial"].toString().first(9).last(8));
        newItem->setFlags(newItem->flags() & ~Qt::ItemIsEditable);
        ui->localViewTable->setItem(currentRowCount + i, 0, newItem);

        QTableWidgetItem *newItem2;
        Equipment * thisEquip = nullptr;
        Ship * thisShip = nullptr;
        bool isEquip = item["def"].toInt() < KP::equipIdMax;
        if(isEquip) {
            thisEquip = engine.getEquipmentReg(item["def"].toInt());
            if(thisEquip->isInvalid()) {
                newItem2 = new QTableWidgetItem(
                    QString::number(item["def"].toInt()));
            }
            else {
                newItem2 = new QTableWidgetItem(
                    thisEquip->toString(settings->value("language", "ja_JP").toString()));
                newItem2->setIcon(Icute::equipIcon(thisEquip->type, false));
            }
        }
        else {
            thisShip = engine.getShipReg(item["def"].toInt());
            if(thisShip->isAmnesiac()) {
                newItem2 = new QTableWidgetItem(
                    QString::number(item["def"].toInt()));
            }
            else {
                newItem2 = new QTableWidgetItem(
                    thisShip->toString(settings->value("language", "ja_JP").toString()));
                newItem2->setIcon(Icute::shipIcon(thisShip->getId(), false));
            }
        }
        newItem2->setFlags(newItem->flags() & ~Qt::ItemIsEditable);
        ui->localViewTable->setItem(currentRowCount + i, 1, newItem2);

        QTableWidgetItem *newItem3 = new TableWidgetItemNumber(
            isEquip ? thisEquip->getTech() : thisShip->getTech());
        newItem3->setFlags(newItem->flags() & ~Qt::ItemIsEditable);
        ui->localViewTable->setItem(currentRowCount + i, 2, newItem3);

        /* item["weight"].toDouble() */
        QTableWidgetItem *newItem4 = new TableWidgetItemNumber(
            item["weight"].toDouble(-1));
        newItem4->setFlags(newItem->flags() & ~Qt::ItemIsEditable);
        ui->localViewTable->setItem(currentRowCount + i, 3, newItem4);

        QTableWidgetItem *newItem5 = new TableWidgetItemNumber(
            isEquip ? thisEquip->type.getTypeSort() : -(thisShip->getId()));
        newItem5->setFlags(newItem->flags() & ~Qt::ItemIsEditable);
        ui->localViewTable->setItem(currentRowCount + i, 4, newItem5);
        ++i;
    }
    ui->localViewTable->setHorizontalHeaderLabels(
        {qtTrId("Serial-num"), qtTrId("Equip-name-def"),
         qtTrId("Equip-tech-level"), qtTrId("Weight")});
    ui->localViewTable->setSortingEnabled(true);
    ui->localViewTable->sortByColumn(4, Qt::DescendingOrder);
    ui->localViewTable->sortByColumn(2, Qt::DescendingOrder);
    ui->localViewTable->hideColumn(4);
    QTimer::singleShot(1, this, [this]{resizeColumns(false);});
}

void TechView::updateSkillPoints(const QJsonObject &djson) {
    ui->skillPointsValue->setText(QString("%1/%2")
                                      .arg(djson["actualSP"].toInteger())
                                      .arg(djson["desiredSP"].toInteger()));
}

void TechView::resizeColumns(bool global) {
    QHeaderView *hH = global ? ui->globalViewTable->horizontalHeader()
                             : ui->localViewTable->horizontalHeader();
    hH->setSectionResizeMode(QHeaderView::ResizeToContents);
    int outerTableWidth = hH->size().width();
    int innerTableWidth = 0;
    for(int i = 0; i < 4; ++i)
        innerTableWidth += hH->sectionSize(hH->logicalIndex(i));
    hH->setSectionResizeMode(QHeaderView::Interactive);
    hH->resizeSection(hH->logicalIndex(1),
                      hH->sectionSize(hH->logicalIndex(1))
                          + outerTableWidth - innerTableWidth);
}

void TechView::resetLocalListName() {
    if(isEquipChoice) {
        ui->localListEquip->clear();
        for(auto &equipReg:
             Clientv2::getInstance().equipRegistryCache) {
            if(
                (ui->localListType1->currentText().
                     localeAwareCompare(qtTrId("all-equipments")) == 0
                 && equipReg->type.getDisplayGroup()
                            .compare("VIRTUAL", Qt::CaseInsensitive) != 0
                 && !equipReg->localNames.value("ja_JP").isEmpty())
                || equipReg->type.getDisplayGroup()
                           .compare(ui->localListType1->currentText(),
                                    Qt::CaseInsensitive) == 0) {
                QString equipName = equipReg->toString(
                    settings->value("language", "ja_JP").toString());
                if(equipName.isEmpty()) {
                    equipName = equipReg->toString("ja_JP");
                }
                ui->localListEquip->addItem(equipName);
            }
        }
    }
    else {
        ui->localListShip->clear();
        auto meta = QMetaEnum::fromType<KP::ShipNationality>();
        bool pass;
        auto sender = QObject::sender();
        QString classText;
        if(sender == ui->localListNation) {
            ui->localListType2->clear();
            ui->localListClass->clear();
        }
        if(sender == ui->localListType2) {
            ui->localListClass->clear();
        }
        for(auto &shipReg:
             Clientv2::getInstance().shipRegistryCache) {
            if(shipReg->isAmnesiac()) {
                continue;
            }
            pass = true;

            if(ui->localListNation->currentText().localeAwareCompare(
                    qtTrId("all-nationality")) != 0
                && qtTrId(meta.key(shipReg->getNationality()))
                           .localeAwareCompare(
                               ui->localListNation->currentText()) != 0) {
                pass = false;
            }
            if(sender != ui->localListType2
                && sender != ui->localListClass) {
                if(pass) {
                    QString type = shipReg->getType().toString();
                    if(ui->localListType2->findText(type) == -1) {
                        ui->localListType2->addItem(type);
                    }
                }
            }

            if(shipReg->getType().toString().localeAwareCompare(
                    ui->localListType2->currentText()) != 0) {
                pass = false;
            }
            classText =
                shipReg->shipClassText[
                    settings->value("language", "ja_JP").toString()
            ];
            if(classText.isEmpty()) {
                classText = shipReg->shipClassText["ja_JP"];
            }
            if(sender != ui->localListClass) {
                if(pass) {
                    if(ui->localListClass->findText(classText) == -1) {
                        ui->localListClass->addItem(classText);
                    }
                }
            }

            if(classText.localeAwareCompare(
                    ui->localListClass->currentText()) != 0) {
                pass = false;
            }

            if(pass) {
                QString shipName = shipReg->toString(
                    settings->value("language", "ja_JP").toString());
                if(shipName.isEmpty()) {
                    shipName = shipReg->toString("ja_JP");
                }
                ui->localListShip->addItem(shipName);
            }
        }
    }
}

void TechView::resizeEvent(QResizeEvent *event) {
    resizeColumns(true);
    resizeColumns(false);
    QWidget::resizeEvent(event);
}

void TechView::showEvent(QShowEvent *event) {
    ui->globalViewTable->setHorizontalHeaderLabels(
        {qtTrId("Serial-num"), qtTrId("Equip-name-def"),
         qtTrId("Equip-tech-level"), qtTrId("Weight")});

    ui->localViewTable->setHorizontalHeaderLabels(
        {qtTrId("Serial-num"), qtTrId("Equip-name-def"),
         qtTrId("Equip-tech-level"), qtTrId("Weight")});
    QWidget::showEvent(event);
}

TableWidgetItemNumber::TableWidgetItemNumber(double content) {
    QTableWidgetItem::setText(QString::number(content));
}
