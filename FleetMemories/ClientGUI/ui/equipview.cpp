#include "equipview.h"
#include "ClientGUI/ui/ui_equipview.h"
#include <QToolButton>
#include <QStyleHints>
#include <QStyledItemDelegate>
#include "../clientv2.h"

extern std::unique_ptr<QSettings> settings;

EquipView::EquipView(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::EquipView)
{
    ui->setupUi(this);

    Clientv2 &engine = Clientv2::getInstance();
    model = &engine.equipModel;

    arsenalView = new QTableView(ui->ArsenalControl);
    arsenalView->setModel(model);
    arsenalView->setObjectName("arsenalview");
    arsenalView->setStyleSheet(
        "QTableView#arsenalview { border-style: none; }");

    QLayout *layoutTop = ui->ArsenalControl->layout();
    layoutTop->addWidget(arsenalView);
    layoutTop->setAlignment(arsenalView, Qt::AlignCenter);
    arsenalView->setMinimumSize(QSize(800,800));

    arsenalView->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    arsenalView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
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

    firstButton->setIcon(first);
    prevButton->setIcon(prev);
    nextButton->setIcon(next);
    lastButton->setIcon(last);

    pageLabel->setAlignment(Qt::AlignCenter);
    pageLabel->setSizePolicy(QSizePolicy(QSizePolicy::Maximum,
                                         QSizePolicy::Preferred,
                                         QSizePolicy::Label));
    pageLabel->resize(QSize(100, pageLabel->size().height()));

    equipSelect = new EquipSelect(20);
    shipSelect = new ShipSelect(20);

    QWidget *layoutWidget = new QWidget(this);
    QHBoxLayout *layout = new QHBoxLayout(layoutWidget);
    layout->addWidget(firstButton);
    layout->addWidget(prevButton);
    layout->addWidget(pageLabel);
    layout->addWidget(nextButton);
    layout->addWidget(lastButton);
    layout->setContentsMargins(0,0,0,0);
    QVBoxLayout *layout2 = ui->Navigator;
    layout2->addWidget(equipSelect, 1, Qt::AlignHCenter);
    layout2->addWidget(shipSelect, 1, Qt::AlignHCenter);
    layout2->addWidget(layoutWidget, 1, Qt::AlignHCenter);
    layout2->setContentsMargins(0,0,0,0);
    layout2->setSpacing(1);

    QSizePolicy labelSize = QSizePolicy(QSizePolicy::Maximum,
                                        QSizePolicy::Maximum);
    equipSelect->setSizePolicy(labelSize);
    shipSelect->setSizePolicy(labelSize);
    layoutWidget->setSizePolicy(labelSize);

    connect(equipSelect, &EquipSelect::typeActivated,
            model, &EquipModel::switchDisplayType);
    connect(equipSelect, &EquipSelect::equipActivated,
            model, &EquipModel::switchDisplayType2);
    connect(equipSelect, &EquipSelect::destructActivated,
            model, &EquipModel::enactDestruct);
    connect(equipSelect, &EquipSelect::searchBoxChanged,
            model, &EquipModel::switchDisplayType2);
    connect(shipSelect, &ShipSelect::selectChanged,
            &engine.shipModel, &ShipModel::switchShipDisplayType);
    connect(&engine.shipModel, &ShipModel::typeBoxHint,
            shipSelect, &ShipSelect::typeBoxHinted);
    connect(&engine.shipModel, &ShipModel::classBoxHint,
            shipSelect, &ShipSelect::classBoxHinted);

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
        Clientv2 &engine = Clientv2::getInstance();
        if(model == &engine.equipModel) {
            //% "No suitable Equipment"
            pageLabel->setText(qtTrId("no-equip"));
        }
        else {
            //% "No suitable Ship"
            pageLabel->setText(qtTrId("no-ship"));
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
    arsenalView
        ->setMinimumSize(QSize(tableSizeWhole(arsenalView,
                                              model).width(),
                               ui->ArsenalControl->size().height()));
    arsenalView->hide();
    arsenalView->show();
}

void EquipView::itemSelected(QUuid id) {
    qCritical() << id;
}


void EquipView::pageNumChangedLambda(int current, int total) {
    columnResized(0, 0, 0);
    update();
}

void EquipView::activate(bool arsenal, bool isEquip) {
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

    Clientv2 &engine = Clientv2::getInstance();
    disconnect(model, SIGNAL(needReCalculateRows()),
               this, SLOT(recalculateArsenalRows()));
    disconnect(this, SIGNAL(rowCountHint(int)),
               model, SLOT(setRowsPerPageHint(int)));
    arsenalView->setItemDelegate(new QStyledItemDelegate());
    if(isEquip) {
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
            model->setIsInArsenal(true);
        }
        else {
            model->setIsInArsenal(false);
            arsenalView->setItemDelegateForColumn(model->selectColumn(),
                                                  delegate);
            connect(delegate, &SelectDelegate::itemSelected,
                    this, &EquipView::itemSelected);
        }
        recalculateArsenalRows();
        connect(model, SIGNAL(needReCalculateRows()),
                this, SLOT(recalculateArsenalRows()),
                Qt::UniqueConnection);
        connect(this, SIGNAL(rowCountHint(int)),
                model, SLOT(setRowsPerPageHint(int)),
                Qt::UniqueConnection);
        equipSelect->show();
        shipSelect->hide();
    }
    else {
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
            model->setIsInArsenal(true);
            arsenalView->setItemDelegateForColumn(model->hpColumn(), hpdelegate);
        }
        else {
            model->setIsInArsenal(false);
            arsenalView->setItemDelegateForColumn(model->selectColumn(),
                                                  delegate);
            arsenalView->setItemDelegateForColumn(model->hpColumn(), hpdelegate);
            connect(delegate, &SelectDelegate::itemSelected,
                    this, &EquipView::itemSelected);
        }
        recalculateArsenalRows();
        connect(model, SIGNAL(needReCalculateRows()),
                this, SLOT(recalculateArsenalRows()),
                Qt::UniqueConnection);
        connect(this, SIGNAL(rowCountHint(int)),
                model, SLOT(setRowsPerPageHint(int)),
                Qt::UniqueConnection);
        equipSelect->hide();
        shipSelect->show();
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
}

void EquipView::recalculateArsenalRows() {
    int rowSize = arsenalView->verticalHeader()->sectionSize(0);
    int rowSizeAvailable = ui->ArsenalControl->size().height()
                           - arsenalView->horizontalHeader()->size().height();
    if(rowSize > 0)
        emit rowCountHint(std::max(rowSizeAvailable / rowSize - 1, 1));
    arsenalView
        ->setMinimumSize(QSize(tableSizeWhole(arsenalView,
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
