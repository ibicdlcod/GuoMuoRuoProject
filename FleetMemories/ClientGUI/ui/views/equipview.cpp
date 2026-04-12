/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "equipview.h"
#include "ui_equipview.h"

#include <QStyleHints>
#include <QStyledItemDelegate>
#include <QToolButton>
#include <QWheelEvent>

#include <optional>

#include "../../clientv2.h"
#include "../../model/shipmodel.h"

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

    sortHint = new QLabel(this);
    //% "Sort Mode:"
    sortHint->setText(qtTrId("sort-hint"));
    sortBox = new QComboBox(this);
    sortBox->setSizePolicy(QSizePolicy(QSizePolicy::Maximum,
                                       QSizePolicy::Preferred,
                                       QSizePolicy::ComboBox));
    sortBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    reverseCheck = new QCheckBox(this);
    //% "Desc."
    reverseCheck->setText(qtTrId("sort-order-desc"));
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

    // Fleet filter radio buttons
    fleetFilterGroup = new QButtonGroup(this);
    //% "All"
    fleetRadioAll = new QRadioButton(qtTrId("all-ships-including-disabled"), this);
    fleetRadio1 = new QRadioButton("1", this);
    fleetRadio2 = new QRadioButton("2", this);
    fleetRadio3 = new QRadioButton("3", this);
    fleetRadio4 = new QRadioButton("4", this);
    //% "U"
    fleetRadioUnassigned = new QRadioButton(qtTrId("fleet-unassigned"), this);
    
    // Set tooltips
    //% "All ships (including disabled)"
    fleetRadioAll->setToolTip(qtTrId("all-ships-including-disabled-tooltip"));
    //% "Fleet %1"
    fleetRadio1->setToolTip(qtTrId("fleet-xx").arg(1));
    fleetRadio2->setToolTip(qtTrId("fleet-xx").arg(2));
    fleetRadio3->setToolTip(qtTrId("fleet-xx").arg(3));
    fleetRadio4->setToolTip(qtTrId("fleet-xx").arg(4));
    //% "Unassigned ships"
    fleetRadioUnassigned->setToolTip(qtTrId("fleet-unassigned-tooltip"));
    
    // Add to button group with IDs
    fleetFilterGroup->addButton(fleetRadioAll, -100);
    fleetFilterGroup->addButton(fleetRadioUnassigned, -1);
    fleetFilterGroup->addButton(fleetRadio1, 0);
    fleetFilterGroup->addButton(fleetRadio2, 1);
    fleetFilterGroup->addButton(fleetRadio3, 2);
    fleetFilterGroup->addButton(fleetRadio4, 3);

    // Load saved fleet filter
    int savedFilter = settings->value("AnchorageFleetFilter", -100).toInt();
    if(savedFilter == -100) {
        fleetRadioAll->setChecked(true);
    } else if(savedFilter == -1) {
        fleetRadioUnassigned->setChecked(true);
    } else if(savedFilter >= 0 && savedFilter <= 3) {
        QAbstractButton *button = fleetFilterGroup->button(savedFilter);
        if(button) button->setChecked(true);
    }

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
    layout->addWidget(fleetRadioAll);
    layout->addWidget(fleetRadio1);
    layout->addWidget(fleetRadio2);
    layout->addWidget(fleetRadio3);
    layout->addWidget(fleetRadio4);
    layout->addWidget(fleetRadioUnassigned);
    layout->addWidget(sortHint);
    layout->addWidget(sortBox);
    layout->addWidget(reverseCheck);
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
    connect(fleetFilterGroup, &QButtonGroup::buttonClicked,
            this, [this](QAbstractButton *button) {
                int id = fleetFilterGroup->id(button);
                std::optional<int> filterValue;
                if(id == -100) {
                    filterValue = std::nullopt;
                } else {
                    filterValue = id;
                }
                Client &engine = Client::getInstance();
                engine.shipModel.setFleetFilter(filterValue);

                // Save to settings
                settings->setValue("AnchorageFleetFilter", id);
                settings->sync();
            });
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
                         std::optional<KP::FactoryState> custom,
                         std::optional<int> sortMode,
                         std::optional<bool> sortReverse) {
    arsenalView->setItemDelegateForColumn(model->selectColumn(),
                                          new QStyledItemDelegate());
    arsenalView->setItemDelegateForColumn(model->hpColumn(),
                                          new QStyledItemDelegate());
    if(ShipModel *sm = qobject_cast<ShipModel *>(model)) {
        arsenalView->setItemDelegateForColumn(sm->fuelColumn(),
                                              new QStyledItemDelegate());
        arsenalView->setItemDelegateForColumn(sm->ammoColumn(),
                                              new QStyledItemDelegate());
    }
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
    disconnect(sortBox, &QComboBox::currentIndexChanged,
               nullptr, nullptr);
    disconnect(reverseCheck, &QCheckBox::toggled,
               nullptr, nullptr);
    disconnect(model, &EquipModel::sortReversedChanged,
               this, nullptr);

    Client &engine = Client::getInstance();

    // Apply saved fleet filter when entering anchorage view
    int savedFilter = settings->value("AnchorageFleetFilter", -100).toInt();
    std::optional<int> filterValue;
    if(savedFilter == -100) {
        filterValue = std::nullopt;
    } else {
        filterValue = savedFilter;
    }
    engine.shipModel.setFleetFilter(filterValue);
    
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
            sortBox->hide();
            reverseCheck->hide();
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
            sortBox->show();
            sortBox->blockSignals(true);
            sortBox->clear();
            //% "Equipment type"
            sortBox->addItem(qtTrId("sort-equip-def"));
            //% "UUID"
            sortBox->addItem(qtTrId("sort-uuid"));
            //% "Name"
            sortBox->addItem(qtTrId("sort-name"));
            //% "Improvement"
            sortBox->addItem(qtTrId("sort-equip-star"));
            //% "Primary attribute"
            sortBox->addItem(qtTrId("sort-equip-prim-attr"));
            //% "Skill points"
            sortBox->addItem(qtTrId("sort-equip-skill"));
            // Load saved sort settings for equipment mode
            int savedSortMode = sortMode.has_value() ? sortMode.value() : settings->value("SortModeEquip", 0).toInt();
            if(savedSortMode < 0 || savedSortMode >= sortBox->count()) {
                savedSortMode = 0;
            }
            sortBox->setCurrentIndex(savedSortMode);
            sortBox->blockSignals(false);
            model->setSortMode(savedSortMode);
            connect(sortBox, &QComboBox::currentIndexChanged,
                    model, &EquipModel::setSortMode);
            // Save sort mode when changed
            connect(sortBox, &QComboBox::currentIndexChanged,
                    this, [](int index) {
                        settings->setValue("SortModeEquip", index);
                        settings->sync();
                    });
            reverseCheck->show();
            reverseCheck->blockSignals(true);
            bool savedReverse = sortReverse.has_value() ? sortReverse.value() : settings->value("SortReversedEquip", false).toBool();
            reverseCheck->setChecked(savedReverse);
            reverseCheck->blockSignals(false);
            model->setSortReversed(savedReverse);
            connect(reverseCheck, &QCheckBox::toggled,
                    model, &EquipModel::setSortReversed);
            // Save reverse setting when changed
            connect(reverseCheck, &QCheckBox::toggled,
                    this, [](bool checked) {
                        settings->setValue("SortReversedEquip", checked);
                        settings->sync();
                    });
            connect(model, &EquipModel::sortReversedChanged,
                    this, [this](bool val) {
                        reverseCheck->blockSignals(true);
                        reverseCheck->setChecked(val);
                        reverseCheck->blockSignals(false);
                    });
            lay->setCurrentWidget(equipSelect);
            // Re-apply the saved Equiptype/Equip filter so the model reflects
            // the user's previous selection even after filterByShip() reset it.
            model->switchDisplayType(equipSelect->typeBox->currentIndex());
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
                    qobject_cast<ShipModel *>(model);
                model->setIsInArsenal(true);
                bool isAnchorage =
                    custom == KP::Anchorage;
                sm->setIsSupplyMode(isAnchorage);
                arsenalView->setItemDelegateForColumn(
                    model->hpColumn(), hpdelegate);
                arsenalView->setItemDelegateForColumn(
                    sm->fuelColumn(), hpdelegate);
                arsenalView->setItemDelegateForColumn(
                    sm->ammoColumn(), hpdelegate);
                shipSelect->addStarButton->show();
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
                qobject_cast<ShipModel *>(model)
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
            sortBox->show();
            sortBox->blockSignals(true);
            sortBox->clear();
            //% "Ship type"
            sortBox->addItem(qtTrId("sort-ship-def"));
            //% "UUID"
            sortBox->addItem(qtTrId("sort-uuid"));
            //% "Name"
            sortBox->addItem(qtTrId("sort-name"));
            //% "Modernization"
            sortBox->addItem(qtTrId("sort-ship-modernization"));
            //% "HP%"
            sortBox->addItem(qtTrId("sort-hp-pct"));
            //% "Condition"
            sortBox->addItem(qtTrId("sort-cond"));
            //% "Level"
            sortBox->addItem(qtTrId("sort-level"));
            //% "Position"
            sortBox->addItem(qtTrId("sort-position"));
            //% "Fuel%"
            sortBox->addItem(qtTrId("sort-fuel-pct"));
            //% "Ammo%"
            sortBox->addItem(qtTrId("sort-ammo-pct"));
            // Load saved sort settings for ship mode
            int savedSortMode = sortMode.has_value() ? sortMode.value() : settings->value("SortModeShip", 0).toInt();
            if(savedSortMode < 0 || savedSortMode >= sortBox->count()) {
                savedSortMode = 0;
            }
            sortBox->setCurrentIndex(savedSortMode);
            sortBox->blockSignals(false);
            model->setSortMode(savedSortMode);
            connect(sortBox, &QComboBox::currentIndexChanged,
                    model, &EquipModel::setSortMode);
            // Save sort mode when changed
            connect(sortBox, &QComboBox::currentIndexChanged,
                    this, [](int index) {
                        settings->setValue("SortModeShip", index);
                        settings->sync();
                    });
            reverseCheck->show();
            reverseCheck->blockSignals(true);
            bool savedReverse = sortReverse.has_value() ? sortReverse.value() : settings->value("SortReversedShip", false).toBool();
            reverseCheck->setChecked(savedReverse);
            reverseCheck->blockSignals(false);
            model->setSortReversed(savedReverse);
            connect(reverseCheck, &QCheckBox::toggled,
                    model, &EquipModel::setSortReversed);
            // Save reverse setting when changed
            connect(reverseCheck, &QCheckBox::toggled,
                    this, [](bool checked) {
                        settings->setValue("SortReversedShip", checked);
                        settings->sync();
                    });
            connect(model, &EquipModel::sortReversedChanged,
                    this, [this](bool val) {
                        reverseCheck->blockSignals(true);
                        reverseCheck->setChecked(val);
                        reverseCheck->blockSignals(false);
                    });
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
            sortBox->hide();
            reverseCheck->hide();
            lay->setCurrentWidget(shipSelect);
            shipSelect->addStarButton->hide();
            shipSelect->decorateButton->hide();
            shipSelect->supplyButton->hide();
            shipSelect->supplyAllButton->hide();
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
