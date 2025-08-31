#include "equipview.h"
#include "ui_equipview.h"
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
    connect(model, &EquipModel::pageNumChanged,
            this, [this](int, int){columnResized(0, 0, 0);
                update();
            });

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


    typebox = new QComboBox(this);
    typebox->setObjectName("typeselect");
    /*
    typebox->setStyleSheet(
        "QComboBox#typeselect { color: palette(base); }");
*/
    equipbox = new QComboBox(this);
    equipbox->setObjectName("equipdef");

    firstButton = new QToolButton(this);
    prevButton = new QToolButton(this);
    pageLabel = new QLabel(this);
    //% "Retrieving data, please wait..."
    pageLabel->setText(qtTrId("retrieving-please-wait"));
    nextButton = new QToolButton(this);
    lastButton = new QToolButton(this);
    destructButton = new QPushButton(this);
    addStarButton = new QPushButton(this);

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
    layout->addWidget(equipbox);
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

    QSizePolicy equipBoxSize = QSizePolicy(QSizePolicy::Maximum,
                                           QSizePolicy::Preferred,
                                           QSizePolicy::ComboBox);
    equipbox->setSizePolicy(equipBoxSize);
    equipbox->resize(QSize(100, pageLabel->size().height()));
    equipbox->setSizeAdjustPolicy(QComboBox::AdjustToContents);

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
    connect(equipbox, &QComboBox::activated,
            model, [this]{
                model->switchDisplayType2(equipbox->currentText());
            });
    connect(typebox, &QComboBox::activated,
            this, &EquipView::reCalculateAvailableEquips);
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

    delegate = new SelectDelegate(arsenalView);
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

void EquipView::itemSelected(QUuid id) {
    qCritical() << id;
}

void EquipView::reCalculateAvailableEquips(int index) {
    Q_UNUSED(index);
    equipbox->clear();
    for(auto &equipReg:
         Clientv2::getInstance().equipRegistryCache) {
        if(
            (typebox->currentText().compare("All equipments") == 0
             && equipReg->type.getDisplayGroup()
                        .compare("VIRTUAL", Qt::CaseInsensitive) != 0
             && !equipReg->localNames.value("ja_JP").isEmpty())
            || equipReg->type.getDisplayGroup()
                       .compare(typebox->currentText(),
                                Qt::CaseInsensitive) == 0) {
            QString equipName = equipReg->toString(
                settings->value("language", "ja_JP").toString());
            if(equipName.isEmpty()) {
                equipName = equipReg->toString("ja_JP");
            }
            equipbox->addItem(equipName);
        }
    }
}

void EquipView::activate(bool arsenal) {
    if(arsenal) {
        arsenalView->setItemDelegate(new QStyledItemDelegate());
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
        arsenalView->setItemDelegateForColumn(model->selectColumn(), delegate);
        recalculateArsenalRows();
        connect(delegate, &SelectDelegate::itemSelected,
                this, &EquipView::itemSelected);
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
    for(int i = 0; i < model->columnCount(); ++i) {
        arsenalView->setColumnHidden(i, i == model->hiddenSortColumn());
    }
}

void EquipView::resizeEvent(QResizeEvent *event) {
    recalculateArsenalRows();
    QWidget::resizeEvent(event);
}
