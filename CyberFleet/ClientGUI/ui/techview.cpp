#include "techview.h"
#include "ui_techview.h"
#include "../clientv2.h"
#include "../networkerror.h"

extern std::unique_ptr<QSettings> settings;

TechView::TechView(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::TechView)
{
    ui->setupUi(this);

    Clientv2 &engine = Clientv2::getInstance();
    QObject::connect(&engine, &Clientv2::receivedGlobalTechInfo,
                     this, &TechView::updateGlobalTech);
    QObject::connect(&engine, &Clientv2::receivedGlobalTechInfo2,
                     this, &TechView::updateGlobalTechViewTable);
    QObject::connect(&engine, &Clientv2::receivedLocalTechInfo,
                     this, &TechView::updateLocalTech);
    QObject::connect(&engine, &Clientv2::receivedLocalTechInfo2,
                     this, &TechView::updateLocalTechViewTable);
    QObject::connect(&engine, &Clientv2::equipRegistryComplete,
                     this->ui->waitText, &QLabel::hide);
    ui->globalViewTable->hide();
    ui->waitText->show();
    ui->waitText->setWordWrap(true);
    ui->waitText->setText(
        QStringLiteral("updating equipment data, please wait..."));
    QHeaderView *horizationalH = ui->globalViewTable->horizontalHeader();
    horizationalH->setSectionResizeMode(QHeaderView::ResizeToContents);
    QHeaderView *verticalH = ui->globalViewTable->verticalHeader();
    verticalH->sectionResizeMode(QHeaderView::ResizeToContents);

    QSet<QString> equipGroups = EquipType::getDisplayGroups();
    QList<QString> sortedGroups;
    for(auto &equip: equipGroups) {
        sortedGroups.append(equip);
    }
    std::sort(sortedGroups.begin(), sortedGroups.end(),
              [](const QString &a, const QString &b){
                  return a.localeAwareCompare(b) < 0;
              });

    for(auto &equipType: sortedGroups) {
        if(equipType.compare("VIRTUAL", Qt::CaseInsensitive) == 0)
            continue;
        ui->localListType->addItem(equipType);
    }
    ui->localListType->addItem("All equipments");
    resetLocalListName(0);
    connect(ui->localListType, &QComboBox::activated,
            this, &TechView::resetLocalListName);
    connect(ui->localListValue, &QComboBox::activated,
            this, &TechView::demandLocalTech);
}

TechView::~TechView()
{
    delete ui;
}

void TechView::demandLocalTech(int index) {
    Q_UNUSED(index)

    ui->localViewTable->clear();

    Clientv2 &engine = Clientv2::getInstance();
    for(auto &equipReg:
         engine.equipRegistryCache) {
        for(auto &name: equipReg->localNames) {
            if(name.compare(ui->localListValue->currentText(),
                             Qt::CaseInsensitive) == 0) {
                engine.socket.flush();
                QByteArray msg = KP::clientDemandGlobalTech(equipReg->getId());
                const qint64 written = engine.socket.write(msg);
                if (written <= 0) {
                    throw NetworkError(engine.socket.errorString());
                }
                return;
            }
        }
    }
}

void TechView::updateGlobalTech(const QJsonObject &djson) {
    ui->globalTechValue->setText(
        QString::number(djson.value("value").toDouble()));
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
    else {
        ui->globalViewTable->show();
        ui->waitText->hide();
    }
    ui->globalViewTable->setHorizontalHeaderLabels(
        {qtTrId("Serial-num"), qtTrId("Equip-name-def"),
         qtTrId("Equip-tech-level"), qtTrId("Weight")});
    ui->globalViewTable->setColumnCount(4);
    QJsonArray contents = djson["content"].toArray();
    int currentRowCount = ui->globalViewTable->rowCount();
    if(djson["initial"].toBool()) {
        ui->globalViewTable->clear();
        currentRowCount = 0;
        ui->globalViewTable->setRowCount(contents.size());
    }
    else {
        ui->globalViewTable->setRowCount(contents.size() + currentRowCount);
    }
    int i = 0;
    for(auto content: contents) {
        QJsonObject item = content.toObject();
        QTableWidgetItem *newItem = new QTableWidgetItem(
            QString::number(item["serial"].toInt()));
        ui->globalViewTable->setItem(currentRowCount + i, 0, newItem);

        QTableWidgetItem *newItem2;
        Equipment * thisEquip = engine.getEquipmentReg(item["def"].toInt());
        if(thisEquip->isInvalid()) {
            newItem2 = new QTableWidgetItem(
                QString::number(item["def"].toInt()));
        }
        else {
            newItem2 = new QTableWidgetItem(
                thisEquip->toString(settings->value("language", "ja_JP").toString()));
        }
        ui->globalViewTable->setItem(currentRowCount + i, 1, newItem2);
        QTableWidgetItem *newItem3 = new TableWidgetItemNumber(
            thisEquip->getTech());
        ui->globalViewTable->setItem(currentRowCount + i, 2, newItem3);
        QTableWidgetItem *newItem4 = new TableWidgetItemNumber(
            item["weight"].toDouble());
        ui->globalViewTable->setItem(currentRowCount + i, 3, newItem4);
        ++i;
    }
    if(djson["final"].toBool()) {
        ui->globalViewTable->sortByColumn(3, Qt::DescendingOrder);
        ui->globalViewTable->sortByColumn(2, Qt::DescendingOrder);
        resizeColumns(true);
    }
}

void TechView::updateLocalTech(const QJsonObject &djson) {
    ui->localTechValue->setText(
        QString::number(djson.value("value").toDouble()));
}

void TechView::updateLocalTechViewTable(const QJsonObject &djson) {
    Clientv2 &engine = Clientv2::getInstance();

    if(!engine.isEquipRegistryCacheGood()) {
        return;
    }
    else {
        ui->localViewTable->show();
    }
    ui->localViewTable->setHorizontalHeaderLabels(
        {qtTrId("Serial-num"), qtTrId("Equip-name-def"),
         qtTrId("Equip-tech-level"), qtTrId("Weight")});
    ui->localViewTable->setColumnCount(4);
    QJsonArray contents = djson["content"].toArray();
    int currentRowCount = ui->localViewTable->rowCount();
    if(djson["initial"].toBool()) {
        ui->localViewTable->clear();
        currentRowCount = 0;
        ui->localViewTable->setRowCount(contents.size());
    }
    else {
        ui->localViewTable->setRowCount(contents.size() + currentRowCount);
    }
    int i = 0;
    for(auto content: contents) {
        QJsonObject item = content.toObject();
        QTableWidgetItem *newItem = new QTableWidgetItem(
            QString::number(item["serial"].toInt()));
        ui->localViewTable->setItem(currentRowCount + i, 0, newItem);

        QTableWidgetItem *newItem2;
        Equipment * thisEquip = engine.getEquipmentReg(item["def"].toInt());
        if(thisEquip->isInvalid()) {
            newItem2 = new QTableWidgetItem(
                QString::number(item["def"].toInt()));
        }
        else {
            newItem2 = new QTableWidgetItem(
                thisEquip->toString(settings->value("language", "ja_JP").toString()));
        }
        ui->localViewTable->setItem(currentRowCount + i, 1, newItem2);
        QTableWidgetItem *newItem3 = new TableWidgetItemNumber(
            thisEquip->getTech());
        ui->localViewTable->setItem(currentRowCount + i, 2, newItem3);
        QTableWidgetItem *newItem4 = new TableWidgetItemNumber(
            item["weight"].toDouble());
        ui->localViewTable->setItem(currentRowCount + i, 3, newItem4);
        ++i;
    }
    if(djson["final"].toBool()) {
        ui->localViewTable->setHorizontalHeaderLabels(
            {qtTrId("Serial-num"), qtTrId("Equip-name-def"),
             qtTrId("Equip-tech-level"), qtTrId("Weight")});
        ui->localViewTable->sortByColumn(3, Qt::DescendingOrder);
        ui->localViewTable->sortByColumn(2, Qt::DescendingOrder);
        resizeColumns(false);
    }
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

void TechView::resetLocalListName(int equiptypeInt) {
    ui->localListValue->clear();
    for(auto &equipReg:
         Clientv2::getInstance().equipRegistryCache) {
        if(
            (ui->localListType->currentText().compare("All equipments") == 0
             && equipReg->type.getTypeGroup()
                        .compare("VIRTUAL", Qt::CaseInsensitive) != 0
             && !equipReg->localNames.value("ja_JP").isEmpty())
            || equipReg->type.getTypeGroup()
                       .compare(ui->localListType->currentText(),
                                Qt::CaseInsensitive) == 0) {
            QString equipName = equipReg->toString(
                settings->value("language", "ja_JP").toString());
            if(equipName.isEmpty()) {
                equipName = equipReg->toString("ja_JP");
            }
            ui->localListValue->addItem(equipName);
        }
    }
}

void TechView::resizeEvent(QResizeEvent *event) {
    resizeColumns(true);
    resizeColumns(false);
    QWidget::resizeEvent(event);
}

TableWidgetItemNumber::TableWidgetItemNumber(double content) {
    QTableWidgetItem::setText(QString::number(content));
}
