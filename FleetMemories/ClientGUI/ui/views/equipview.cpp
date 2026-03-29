/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "equipview.h"
#include "ui_equipview.h"

#include <QStyleHints>
#include <QStyledItemDelegate>
#include <QToolButton>
#include <QWheelEvent>

#include "../../clientv2.h"

using namespace std::chrono_literals;

extern std::unique_ptr<QSettings> settings;

EquipView::EquipView(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::EquipView)
{
    ui->setupUi(this);

    Client &engine = Client::getInstance();
    model = &engine.equipModel;

    arsenalView = new QTableView(ui->ArsenalControl);
    arsenalView->setModel(model);
    arsenalView->setObjectName("arsenalview");
    arsenalView->setStyleSheet(
        "QTableView#arsenalview { border-style: none; }");

    QLayout *layoutTop = ui->ArsenalControl->layout();
    layoutTop->addWidget(arsenalView);
    layoutTop->setAlignment(arsenalView, Qt::AlignCenter);
    arsenalView->setMinimumSize(QSize(100,100));

    arsenalView->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    arsenalView->horizontalHeader()->setSectionResizeMode(
        QHeaderView::ResizeToContents);

    columnResizeDebounce = new QTimer(this);
    columnResizeDebounce->setSingleShot(true);
    columnResizeDebounce->setInterval(100ms);
    connect(columnResizeDebounce, &QTimer::timeout,
            this, [this]() {
        arsenalView->setMinimumSize(
            QSize(tableSizeWhole(arsenalView, model).width(),
                  ui->ArsenalControl->size().height()));
        arsenalView->hide();
        arsenalView->show();
    });
    connect(arsenalView->horizontalHeader(), &QHeaderView::sectionResized,
            this, &EquipView::columnResized);

    arsenalView->show();
    /* navigator part */
    QIcon first = QIcon(":/resources/navigation/first.svg");
    QIcon last = QIcon(":/resources/navigation/last.svg");
    QIcon prev = QIcon(":/resources/navigation/prev.svg");
    QIcon next = QIcon(":/resources/navigation/next.svg");
    switch(QApplication::styleHints()->colorScheme()) {
    case Qt::ColorScheme::Dark:
        break;
    case Qt::ColorScheme::Light: [[fallthrough]];
    default:
        first = QIcon(":/resources/navigation/first_dark.svg");
        last = QIcon(":/resources/navigation/last_dark.svg");
        prev = QIcon(":/resources/navigation/prev_dark.svg");
        next = QIcon(":/resources/navigation/next_dark.svg");
        break;
    }

    firstButton = new QToolButton(this);
    prevButton = new QToolButton(this);
    pageLabel = new QLabel(this);
    //% "Retrieving data, please wait..."
    pageLabel->setText(qtTrId("retrieving-please-wait"));
    nextButton = new QToolButton(this);
    lastButton = new QToolButton(this);
    unselectButton = new QPushButton(this);

    firstButton->setIcon(first);
    prevButton->setIcon(prev);
    nextButton->setIcon(next);
    lastButton->setIcon(last);
    //% "Unselect"
    unselectButton->setText(qtTrId("equipview-unselect"));

    pageLabel->setAlignment(Qt::AlignCenter);
    pageLabel->setSizePolicy(QSizePolicy(QSizePolicy::Maximum,
                                         QSizePolicy::Preferred,
                                         QSizePolicy::Label));
    pageLabel->resize(QSize(100, pageLabel->size().height()));

    equipSelect = new EquipSelect(20);
    shipSelect = new ShipSelect(20);
    industrialSelect = new IndustrialSelect(20);

    QWidget *layoutWidget = new QWidget(this);
    QHBoxLayout *layout = new QHBoxLayout(layoutWidget);
    layout->addWidget(firstButton);
    layout->addWidget(prevButton);
    layout->addWidget(pageLabel);
    layout->addWidget(nextButton);
    layout->addWidget(lastButton);
    layout->addWidget(unselectButton);
    layout->setContentsMargins(0,0,0,0);
    lay = new QStackedLayout();
    QVBoxLayout *layout2 = ui->Navigator;
    layout2->addLayout(lay, 1);
    lay->addWidget(equipSelect);
    lay->addWidget(shipSelect);
    lay->addWidget(industrialSelect);
    layout2->addWidget(layoutWidget, 1, Qt::AlignHCenter);
    layout2->setContentsMargins(0,0,0,0);
    layout2->setSpacing(1);

    QSizePolicy labelSize = QSizePolicy(QSizePolicy::Maximum,
                                        QSizePolicy::Maximum);
    equipSelect->setSizePolicy(labelSize);
    shipSelect->setSizePolicy(labelSize);
    industrialSelect->setSizePolicy(labelSize);
    layoutWidget->setSizePolicy(labelSize);

    connect(equipSelect, &EquipSelect::typeActivated,
            model, &EquipModel::switchDisplayType);
    connect(equipSelect, &EquipSelect::equipActivated,
            model, &EquipModel::switchDisplayType2);
    connect(equipSelect, &EquipSelect::destructActivated,
            model, &EquipModel::enactDestruct);
    connect(equipSelect, &EquipSelect::searchBoxChanged,
            model, &EquipModel::switchDisplayType2);
    connect(equipSelect, &EquipSelect::improveActivated,
            model, &EquipModel::enactModernize);

    connect(shipSelect, &ShipSelect::selectChanged,
            &engine.shipModel, &ShipModel::switchShipDisplayType);
    connect(&engine.shipModel, &ShipModel::typeBoxHint,
            shipSelect, &ShipSelect::typeBoxHinted);
    connect(&engine.shipModel, &ShipModel::classBoxHint,
            shipSelect, &ShipSelect::classBoxHinted);
    connect(shipSelect, &ShipSelect::modernizeActivated,
            &engine.shipModel, &ShipModel::enactModernize);
    connect(shipSelect, &ShipSelect::decorateActivated,
            &engine.shipModel, &ShipModel::enactDecorate);

    connect(shipSelect, &ShipSelect::selectChanged,
            &engine.shipBPModel, &ShipModel::switchShipDisplayType);
    connect(&engine.shipBPModel, &ShipModel::typeBoxHint,
            shipSelect, &ShipSelect::typeBoxHinted);
    connect(&engine.shipBPModel, &ShipModel::classBoxHint,
            shipSelect, &ShipSelect::classBoxHinted);

    connect(&engine, &Client::receivedRankInfoUser,
            industrialSelect, &IndustrialSelect::setIPValue);
    connect(industrialSelect, &IndustrialSelect::buyActivated,
            this, &EquipView::buyActivated);
    /*
    connect(&engine, &Clientv2::uiRefreshSig,
            this, &EquipView::recalculateArsenalRows);
*/
    delegate = new SelectDelegate(arsenalView);
    hpdelegate = new HpDelegate(arsenalView);
}

EquipView::~EquipView()
{
    delete ui;
    delete delegate;
}

void EquipView::enactPageNumChange(int currentPageNum, int totalPageNum) {
    if(currentPageNum == 0) {
        firstButton->setEnabled(false);
        prevButton->setEnabled(false);
    }
    else {
        firstButton->setEnabled(true);
        prevButton->setEnabled(true);
    }
    if(currentPageNum == totalPageNum - 1 || totalPageNum == 0) {
        nextButton->setEnabled(false);
        lastButton->setEnabled(false);
    }
    else {
        nextButton->setEnabled(true);
        lastButton->setEnabled(true);
    }
    if(totalPageNum == 0) {
        Client &engine = Client::getInstance();
        if(model == &engine.equipModel) {
            //% "No suitable Equipment"
            pageLabel->setText(qtTrId("no-equip"));
        }
        else if(model == &engine.shipModel
                 || model == &engine.shipBPModel) {
            //% "No suitable Ship"
            pageLabel->setText(qtTrId("no-ship"));
        }
        else {
            //% "No suitable User"
            pageLabel->setText(qtTrId("no-user"));
        }
        return;
    }
    pageLabel->setText(QString::number(currentPageNum + 1)
                       + " / "
                       + QString::number(totalPageNum));
}

void EquipView::columnResized(int logicalIndex, int oldSize, int newSize) {
    Q_UNUSED(logicalIndex)
    Q_UNUSED(oldSize)
    Q_UNUSED(newSize)
    columnResizeDebounce->start(); // restarts if already running
}

void EquipView::itemSelected(QUuid id) {
    Client &engine = Client::getInstance();
    if(model == &engine.equipModel) {
        emit equipSelected(id);
    }
    else {
        emit shipSelected(id);
    }
    hide();
}


void EquipView::pageNumChangedLambda(int current, int total) {
    columnResized(0, 0, 0);
    update();
}

void EquipView::activate(bool arsenal, bool isEquip,
                         std::optional<KP::FactoryState> custom) {
    arsenalView->setItemDelegateForColumn(model->selectColumn(),
                                          new QStyledItemDelegate());
    arsenalView->setItemDelegateForColumn(model->hpColumn(),
                                          new QStyledItemDelegate());
    disconnect(model, &EquipModel::pageNumChanged,
               this, &EquipView::pageNumChangedLambda);
    disconnect(firstButton, &QAbstractButton::clicked,
               model, &EquipModel::firstPage);
    disconnect(prevButton, &QAbstractButton::clicked,
               model, &EquipModel::prevPage);
    disconnect(nextButton, &QAbstractButton::clicked,
               model, &EquipModel::nextPage);
    disconnect(lastButton, &QAbstractButton::clicked,
               model, &EquipModel::lastPage);
    disconnect(model, &EquipModel::pageNumChanged,
               this, &EquipView::enactPageNumChange);
    disconnect(delegate, &SelectDelegate::itemSelected,
               this, &EquipView::itemSelected);
    disconnect(unselectButton, &QPushButton::clicked,
               nullptr, nullptr);

    Client &engine = Client::getInstance();
    disconnect(model, SIGNAL(needReCalculateRows()),
               this, SLOT(recalculateArsenalRows()));
    disconnect(this, SIGNAL(rowCountHint(int)),
               model, SLOT(setRowsPerPageHint(int)));
    arsenalView->setItemDelegate(new QStyledItemDelegate());
    if(arsenal) {
        unselectButton->hide();
    }
    if(isEquip) {
        if(custom == KP::RankView) {
        rank_view:
            model = &engine.rankModel;
            arsenalView->setModel(model);
            connect(model, SIGNAL(needReCalculateRows()),
                    this, SLOT(recalculateArsenalRows()),
                    Qt::UniqueConnection);
            connect(this, SIGNAL(rowCountHint(int)),
                    model, SLOT(setRowsPerPageHint(int)),
                    Qt::UniqueConnection);
            engine.doRefreshRank(rowCountHintVal);
            if(!model->isReady()) {
                pageLabel->setText(qtTrId("retrieving-please-wait"));
                arsenalView->hide();
                connect(model, &EquipModel::equipReady,
                        this, [this](){
                            arsenalView->show();
                            recalculateArsenalRows();
                        });
            }
            else {
                arsenalView->show();
            }
            lay->setCurrentWidget(industrialSelect);
        }
        else {
        equip:
            model = &engine.equipModel;
            arsenalView->setModel(model);
            if(!model->isReady()) {
                pageLabel->setText(qtTrId("retrieving-please-wait"));
                engine.doRefreshFactoryArsenal();
                arsenalView->hide();
            }
            else {
                arsenalView->show();
            }
            if(arsenal) {
                model->filterByShip(nullptr, false);
                model->setIsInArsenal(true);
                model->unsetShip();
                equipSelect->destructButton->show();
                equipSelect->addStarButton->show();
            }
            else {
                model->setIsInArsenal(false);
                arsenalView->setItemDelegateForColumn(model->selectColumn(),
                                                      delegate);
                connect(delegate, &SelectDelegate::itemSelected,
                        this, &EquipView::itemSelected);
                equipSelect->destructButton->hide();
                equipSelect->addStarButton->hide();
                unselectButton->show();
                connect(unselectButton, &QPushButton::clicked,
                        this, [this]{emit equipSelected(QUuid());
                                 hide();});
            }
            connect(model, SIGNAL(needReCalculateRows()),
                    this, SLOT(recalculateArsenalRows()),
                    Qt::UniqueConnection);
            connect(this, SIGNAL(rowCountHint(int)),
                    model, SLOT(setRowsPerPageHint(int)),
                    Qt::UniqueConnection);
            recalculateArsenalRows();
            lay->setCurrentWidget(equipSelect);
        }
    }
    else {
        if(custom != KP::BlueprintView) {
        ship:
            model = &engine.shipModel;
            arsenalView->setModel(model);
            if(!model->isReady()) {
                pageLabel->setText(qtTrId("retrieving-please-wait"));
                engine.doRefreshFactoryAnchorage();
                arsenalView->hide();
            }
            else {
                arsenalView->show();
            }
            if(arsenal) {
                ShipModel *sm =
                    static_cast<ShipModel *>(model);
                model->setIsInArsenal(true);
                bool isAnchorage =
                    custom == KP::Anchorage;
                sm->setIsSupplyMode(isAnchorage);
                arsenalView->setItemDelegateForColumn(
                    model->hpColumn(), hpdelegate);
                shipSelect->addStarButton->setVisible(!isAnchorage);
                shipSelect->decorateButton->show();
                shipSelect->supplyButton->setVisible(isAnchorage);
                shipSelect->supplyAllButton->setVisible(isAnchorage);
                if(isAnchorage) {
                    connect(
                        shipSelect,
                        &ShipSelect::supplyActivated,
                        sm, &ShipModel::enactSupply,
                        Qt::UniqueConnection);
                    connect(
                        shipSelect,
                        &ShipSelect::supplyAllActivated,
                        sm, &ShipModel::enactSupplyAll,
                        Qt::UniqueConnection);
                    connect(
                        sm, &ShipModel::supplyRequest,
                        &engine, &Client::doSupplyShip,
                        Qt::UniqueConnection);
                }
            }
            else {
                model->setIsInArsenal(false);
                static_cast<ShipModel *>(model)
                    ->setIsSupplyMode(false);
                arsenalView->setItemDelegateForColumn(
                    model->selectColumn(), delegate);
                arsenalView->setItemDelegateForColumn(
                    model->hpColumn(), hpdelegate);
                connect(delegate, &SelectDelegate::itemSelected,
                        this, &EquipView::itemSelected);
                shipSelect->addStarButton->hide();
                shipSelect->decorateButton->hide();
                shipSelect->supplyButton->hide();
                shipSelect->supplyAllButton->hide();
                unselectButton->show();
                connect(unselectButton, &QPushButton::clicked,
                        this, [this]{emit shipSelected(QUuid());
                                 hide();});
            }
            connect(model, SIGNAL(needReCalculateRows()),
                    this, SLOT(recalculateArsenalRows()),
                    Qt::UniqueConnection);
            connect(this, SIGNAL(rowCountHint(int)),
                    model, SLOT(setRowsPerPageHint(int)),
                    Qt::UniqueConnection);
            recalculateArsenalRows();
            lay->setCurrentWidget(shipSelect);
        }
        else {
        blueprint_view:
            model = &engine.shipBPModel;
            arsenalView->setModel(model);
            if(!model->isReady()) {
                pageLabel->setText(qtTrId("retrieving-please-wait"));
                engine.doRefreshFactoryAnchorage();
                arsenalView->hide();
            }
            else {
                arsenalView->show();
            }
            model->setIsInArsenal(true);
            arsenalView->setItemDelegateForColumn(
                model->hpColumn(), hpdelegate);
            connect(model, SIGNAL(needReCalculateRows()),
                    this, SLOT(recalculateArsenalRows()),
                    Qt::UniqueConnection);
            connect(this, SIGNAL(rowCountHint(int)),
                    model, SLOT(setRowsPerPageHint(int)),
                    Qt::UniqueConnection);
            recalculateArsenalRows();
            lay->setCurrentWidget(shipSelect);
        }
    }
    connect(model, &EquipModel::pageNumChanged,
            this, &EquipView::pageNumChangedLambda);
    connect(firstButton, &QAbstractButton::clicked,
            model, &EquipModel::firstPage);
    connect(prevButton, &QAbstractButton::clicked,
            model, &EquipModel::prevPage);
    connect(nextButton, &QAbstractButton::clicked,
            model, &EquipModel::nextPage);
    connect(lastButton, &QAbstractButton::clicked,
            model, &EquipModel::lastPage);
    connect(model, &EquipModel::pageNumChanged,
            this, &EquipView::enactPageNumChange);
    if(model->isReady() && arsenal && custom != KP::RankView) {
        model->firstPage();
    }
}

void EquipView::recalculateArsenalRows() {
    int rowSize = arsenalView->verticalHeader()->sectionSize(0);
    int rowSizeAvailable = ui->ArsenalControl->size().height()
                           - arsenalView->horizontalHeader()->size().height();
    if(rowSize > 0) {
        emit rowCountHint(std::max(rowSizeAvailable / rowSize - 1, 1));
        rowCountHintVal = std::max(rowSizeAvailable / rowSize - 1, 1);
    }
    arsenalView
        ->setMinimumSize(QSize(tableSizeWhole(arsenalView,
                                              model).width(),
                               ui->ArsenalControl->size().height()));
    arsenalView
        ->setMaximumSize(QSize(tableSizeWhole(arsenalView,
                                              model).width(),
                               ui->ArsenalControl->size().height()));
    arsenalView->show();
    arsenalView->sortByColumn(model->hiddenSortColumn(), Qt::AscendingOrder);
    for(int i = 0; i < model->columnCount(); ++i) {
        arsenalView->setColumnHidden(i, i == model->hiddenSortColumn());
    }
}

void EquipView::resizeEvent(QResizeEvent *event) {
    recalculateArsenalRows();
    QWidget::resizeEvent(event);
}

void EquipView::closeEvent(QCloseEvent *event) {
    disconnect(this, &EquipView::shipSelected,
               nullptr, nullptr);
    QWidget::closeEvent(event);
}

void EquipView::wheelEvent(QWheelEvent *event) {
    event->accept();
    auto value = event->angleDelta().y();
    if(value > 0 && prevButton->isEnabled()) {
        model->prevPage();
    }
    if(value < 0 && nextButton->isEnabled()) {
        model->nextPage();
    }
}
