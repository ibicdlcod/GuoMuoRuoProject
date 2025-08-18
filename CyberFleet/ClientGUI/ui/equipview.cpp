#include "equipview.h"
#include "ui_equipview.h"
#include <QToolButton>
#include <QStyleHints>
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
    arsenalView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
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


    typebox = new QComboBox();
    typebox->setObjectName("typeselect");
    /*
    typebox->setStyleSheet(
        "QComboBox#typeselect { color: palette(base); }");
*/
    firstButton = new QToolButton();
    prevButton = new QToolButton();
    pageLabel = new QLabel();
    //% "Retrieving data, please wait..."
    pageLabel->setText(qtTrId("retrieving-please-wait"));
    nextButton = new QToolButton();
    lastButton = new QToolButton();
    destructButton = new QPushButton();
    addStarButton = new QPushButton();

    firstButton->setIcon(first);
    prevButton->setIcon(prev);
    nextButton->setIcon(next);
    lastButton->setIcon(last);
    //% "Destruct"
    destructButton->setText(qtTrId("destruct-button"));
    //% "Improve"
    addStarButton->setText(qtTrId("add-star-button"));

    QHBoxLayout *layout = ui->Navigator;
    layout->addWidget(typebox);
    layout->addWidget(firstButton);
    layout->addWidget(prevButton);
    layout->addWidget(pageLabel);
    layout->addWidget(nextButton);
    layout->addWidget(lastButton);
    layout->addWidget(destructButton);
    layout->addWidget(addStarButton);

    typebox->setSizePolicy(QSizePolicy(QSizePolicy::Maximum,
                                       QSizePolicy::Preferred,
                                       QSizePolicy::ComboBox));
    typebox->resize(QSize(100, pageLabel->size().height()));
    //% "All equipments"
    typebox->addItem(qtTrId("all-equipments"));
    typebox->addItems(EquipType::getDisplayGroupsSorted());
    pageLabel->setAlignment(Qt::AlignCenter);
    pageLabel->setSizePolicy(QSizePolicy(QSizePolicy::Maximum,
                                         QSizePolicy::Preferred,
                                         QSizePolicy::Label));
    pageLabel->resize(QSize(100, pageLabel->size().height()));
    destructButton->setSizePolicy(QSizePolicy(QSizePolicy::Maximum,
                                              QSizePolicy::Preferred,
                                              QSizePolicy::PushButton));
    destructButton->resize(QSize(100, pageLabel->size().height()));
    addStarButton->setSizePolicy(QSizePolicy(QSizePolicy::Maximum,
                                             QSizePolicy::Preferred,
                                             QSizePolicy::PushButton));
    addStarButton->resize(QSize(100, pageLabel->size().height()));

    connect(typebox, &QComboBox::activated,
            model, &EquipModel::switchDisplayType);
    connect(firstButton, &QAbstractButton::clicked,
            model, &EquipModel::firstPage);
    connect(prevButton, &QAbstractButton::clicked,
            model, &EquipModel::prevPage);
    connect(nextButton, &QAbstractButton::clicked,
            model, &EquipModel::nextPage);
    connect(lastButton, &QAbstractButton::clicked,
            model, &EquipModel::lastPage);
    connect(destructButton, &QAbstractButton::clicked,
            model, &EquipModel::enactDestruct);
    connect(model, &EquipModel::pageNumChanged,
            this, &EquipView::enactPageNumChange);
}

EquipView::~EquipView()
{
    delete ui;
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
        destructButton->show();
        addStarButton->show();
    }
    else {
        model->setIsInArsenal(false);
        destructButton->hide();
        addStarButton->hide();
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

void EquipView::resizeEvent(QResizeEvent *event) {
    recalculateArsenalRows();
    QWidget::resizeEvent(event);
}
