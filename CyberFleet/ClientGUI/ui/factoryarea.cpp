#include "factoryarea.h"
#include "ui_factoryarea.h"
#include <QHeaderView>
#include <QSvgWidget>
#include <QToolButton>
#include "../clientv2.h"
#include "developwindow.h"
#include "navigator.h"
#include "equipview.h"

FactoryArea::FactoryArea(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::FactoryArea)
{
    ui->setupUi(this);
    equipview = new EquipView(ui->ArsenalArea);
    arsenalView = new QTableView(this);
    arsenalView->setObjectName("arsenalview");
    arsenalView->setStyleSheet(
        "QTableView#arsenalview { border-style: none; }");
    ui->Slots->setObjectName("slotcontrol");
    ui->Slots->setStyleSheet(
        "QFrame#slotcontrol { border-style: none; }");

    QGridLayout * layout = new QGridLayout;
    layout->addWidget(arsenalView);
    layout->setAlignment(arsenalView, Qt::AlignCenter);
    arsenalView->setMinimumSize(QSize(800,800));

    ui->ArsenalControl->setLayout(layout);
    ui->ArsenalControl->show();
    navigator = new Navi(ui->Navigator, getEquipModel());

    arsenalView->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    connect(arsenalView->horizontalHeader(), &QHeaderView::sectionResized,
            this, &FactoryArea::columnResized);
    connect(getEquipModel(), &EquipModel::pageNumChanged,
            this, [this](int, int){
                QTimer::singleShot(10, this,
                                   [this](){columnResized(0, 0, 0);});});

    Clientv2 &engine = Clientv2::getInstance();
    connect(&engine, &Clientv2::receivedFactoryRefresh,
            this, &FactoryArea::doFactoryRefresh);

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
        connect((*iter), &FactorySlot::clickedSpec,
                this, &FactoryArea::developClicked);
        (*iter)->setSlotnum(iter - slotfs.begin());
        (*iter)->setStatus();
    }
}

FactoryArea::~FactoryArea()
{
    delete ui;
}

void FactoryArea::columnResized(int logicalIndex, int oldSize, int newSize) {
    Q_UNUSED(logicalIndex)
    Q_UNUSED(oldSize)
    Q_UNUSED(newSize)
    arsenalView
        ->setMinimumSize(QSize(tableSizeWhole(arsenalView,
                                              getEquipModel()).width(),
                               ui->ArsenalControl->size().height()));
    arsenalView->hide();
    arsenalView->show();
}

void FactoryArea::developClicked(bool checked, int slotnum) {
    Q_UNUSED(checked)

    Clientv2 &engine = Clientv2::getInstance();
    if(factoryState == KP::Development) {
        if(slotfs[slotnum]->isComplete()) {
            engine.doFetch({"fetch", QString::number(slotnum)});
        }
        else if(slotfs[slotnum]->isOnJob()) {
            return;
        }
        else {
            DevelopWindow w;
            if(w.exec() == QDialog::Rejected)
                qDebug() << "NODEVELOP";
            else {
                QTimer::singleShot(100, &engine, &Clientv2::doRefreshFactory);
                QString msg = QStringLiteral("develop %1 %2")
                                  .arg(w.equipIdDesired()).arg(slotnum);
                qDebug() << msg;
                engine.parse(msg);
            }
        }
    }
    else {

    }
    engine.doRefreshFactory();
}

void FactoryArea::doFactoryRefresh(const QJsonObject &input) {
    qDebug("FACTORYREFRESH");
    QJsonArray content = input["content"].toArray();
    for(int i = 0; i < content.size(); ++i) {
        slotfs[i]->setOpen(true);
        QJsonObject item = content[i].toObject();
        if(!item["done"].toBool()) {
            slotfs[i]->setComplete(false);
            if(!item.contains("completetime")
                || item["completetime"].toInt() == 0) {
                slotfs[i]->setCompleteTime(QDateTime());
            }
            else {
                slotfs[i]->setCompleteTime(
                    QDateTime::fromSecsSinceEpoch(
                        item["completetime"].toInt(), QTimeZone::UTC));
            }
        } else {
            slotfs[i]->setComplete(true);
        }
        slotfs[i]->setStatus();
    }
}

EquipModel * FactoryArea::getEquipModel() {
    Clientv2 &engine = Clientv2::getInstance();
    return &engine.equipModel;
}

void FactoryArea::setDevelop(KP::FactoryState state) {
    factoryState = state;
}

void FactoryArea::switchToDevelop() {
    switch(factoryState) {
    case KP::Development:
        ui->FactoryLabel->setText(qtTrId("develop-equipment"));
        ui->Slots->show();
        ui->ArsenalControl->hide();
        ui->NavigatorContol->hide();
        ui->ArsenalArea->hide();
        break;
    case KP::Construction:
        ui->FactoryLabel->setText(qtTrId("construct-ships"));
        ui->Slots->show();
        ui->ArsenalControl->hide();
        ui->NavigatorContol->hide();
        ui->ArsenalArea->hide();
        break;
    case KP::Arsenal:
        ui->FactoryLabel->setText(qtTrId("arsenal"));
        ui->Slots->hide();
        ui->ArsenalControl->hide();
        ui->NavigatorContol->hide();
        /*
        ui->ArsenalControl->show();
        ui->NavigatorContol->show();
        getEquipModel()->setIsInArsenal(true);
        arsenalView->setModel(getEquipModel());
        recalculateArsenalRows();
        connect(getEquipModel(), SIGNAL(needReCalculateRows()),
                this, SLOT(recalculateArsenalRows()),
                Qt::UniqueConnection);
        connect(this, SIGNAL(rowCountHint(int)),
                getEquipModel(), SLOT(setRowsPerPageHint(int)),
                Qt::UniqueConnection);
        if(!getEquipModel()->isReady()) {
            Clientv2 &engine = Clientv2::getInstance();
            engine.doRefreshFactoryArsenal();
            arsenalView->hide();
        }
        else {
            arsenalView->show();
        }
*/
        ui->ArsenalArea->show();
        QTimer::singleShot(10, this, [this](){
            equipview->setGeometry(ui->ArsenalArea->rect());
        });
        equipview->activate(true);
        break;
    }
}

void FactoryArea::recalculateArsenalRows() {
    Clientv2 &engine = Clientv2::getInstance();
    int rowSize = arsenalView->verticalHeader()->sectionSize(0);
    int rowSizeAvailable = ui->ArsenalControl->size().height()
                           - arsenalView->horizontalHeader()->size().height();
    if(rowSize > 0)
        emit rowCountHint(std::max(rowSizeAvailable / rowSize - 1, 1));
    arsenalView
        ->setMinimumSize(QSize(tableSizeWhole(arsenalView,
                                              getEquipModel()).width(),
                               ui->ArsenalControl->size().height()));
    arsenalView->show();
    arsenalView->sortByColumn(getEquipModel()->hiddenSortColumn(), Qt::AscendingOrder);
    arsenalView->setColumnHidden(getEquipModel()->hiddenSortColumn(), true);
}

void FactoryArea::resizeEvent(QResizeEvent *event) {
    recalculateArsenalRows();
    QWidget::resizeEvent(event);
}

