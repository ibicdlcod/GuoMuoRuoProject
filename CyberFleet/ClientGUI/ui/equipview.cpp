#include "equipview.h"
#include "ui_equipview.h"
#include <QToolButton>
#include "../clientv2.h"

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
    connect(arsenalView->horizontalHeader(), &QHeaderView::sectionResized,
            this, &EquipView::columnResized);
    connect(model, &EquipModel::pageNumChanged,
            this, [this](int, int){
                QTimer::singleShot(10, this,
                                   [this](){columnResized(0, 0, 0);});});

    arsenalView->show();
    /* navigator part */
    QIcon first = QIcon(":/resources/navigation/first.svg");
    QIcon last = QIcon(":/resources/navigation/last.svg");
    QIcon prev = QIcon(":/resources/navigation/prev.svg");
    QIcon next = QIcon(":/resources/navigation/next.svg");

    typebox = new QComboBox();
    firstbutton = new QToolButton();
    prevbutton = new QToolButton();
    pageLabel = new QLabel();
    //% "Retrieving data, please wait..."
    pageLabel->setText(qtTrId("retrieving-please-wait"));
    nextbutton = new QToolButton();
    lastbutton = new QToolButton();

    firstbutton->setIcon(first);
    prevbutton->setIcon(prev);
    nextbutton->setIcon(next);
    lastbutton->setIcon(last);

    QHBoxLayout *layout = ui->Navigator;
    layout->addWidget(typebox);
    layout->addWidget(firstbutton);
    layout->addWidget(prevbutton);
    layout->addWidget(pageLabel);
    layout->addWidget(nextbutton);
    layout->addWidget(lastbutton);

    typebox->setSizePolicy(QSizePolicy(QSizePolicy::Maximum,
                                       QSizePolicy::Preferred,
                                       QSizePolicy::ComboBox));
    typebox->resize(QSize(100, pageLabel->size().height()));
    typebox->addItem(qtTrId("all-equipments"));
    typebox->addItems(EquipType::getDisplayGroupsSorted());
    pageLabel->setAlignment(Qt::AlignCenter);
    pageLabel->setSizePolicy(QSizePolicy(QSizePolicy::Maximum,
                                         QSizePolicy::Preferred,
                                         QSizePolicy::Label));
    pageLabel->resize(QSize(100, pageLabel->size().height()));

    connect(typebox, &QComboBox::activated,
            model, &EquipModel::switchDisplayType);
    connect(firstbutton, &QAbstractButton::clicked,
            model, &EquipModel::firstPage);
    connect(prevbutton, &QAbstractButton::clicked,
            model, &EquipModel::prevPage);
    connect(nextbutton, &QAbstractButton::clicked,
            model, &EquipModel::nextPage);
    connect(lastbutton, &QAbstractButton::clicked,
            model, &EquipModel::lastPage);
    connect(model, &EquipModel::pageNumChanged,
            this, &EquipView::enactPageNumChange);
}

EquipView::~EquipView()
{
    delete ui;
}

void EquipView::enactPageNumChange(int currentPageNum, int totalPageNum) {
    if(currentPageNum == 0) {
        firstbutton->setEnabled(false);
        prevbutton->setEnabled(false);
    }
    else {
        firstbutton->setEnabled(true);
        prevbutton->setEnabled(true);
    }
    if(currentPageNum == totalPageNum - 1) {
        nextbutton->setEnabled(false);
        lastbutton->setEnabled(false);
    }
    else {
        nextbutton->setEnabled(true);
        lastbutton->setEnabled(true);
    }
    if(totalPageNum == 0) {
        //% "No suitable Equipment"
        pageLabel->setText(qtTrId("no-equip"));
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

void EquipView::activate(bool arsenal) {
    if(arsenal) {
        model->setIsInArsenal(true);
        recalculateArsenalRows();
        connect(model, SIGNAL(needReCalculateRows()),
                this, SLOT(recalculateArsenalRows()),
                Qt::UniqueConnection);
        connect(this, SIGNAL(rowCountHint(int)),
                model, SLOT(setRowsPerPageHint(int)),
                Qt::UniqueConnection);
        if(!model->isReady()) {
            Clientv2 &engine = Clientv2::getInstance();
            engine.doRefreshFactoryArsenal();
            arsenalView->hide();
        }
        else {
            arsenalView->show();
        }
    }
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
    arsenalView->setColumnHidden(model->hiddenSortColumn(), true);
}
