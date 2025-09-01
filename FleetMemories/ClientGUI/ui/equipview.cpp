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

    searchLabel = new QLabel(this);
    //% "Search:"
    searchLabel->setText(qtTrId("equipview-search"));
    searchBox = new QLineEdit(this);
    searchBox->setObjectName("searchbox");
    typeLabel = new QLabel(this);
    //% "Equip type:"
    typeLabel->setText(qtTrId("equipview-type"));
    typeBox = new QComboBox(this);
    typeBox->setObjectName("typeselect");
    /*
    typebox->setStyleSheet(
        "QComboBox#typeselect { color: palette(base); }");
*/
    equipLabel = new QLabel(this);
    //% "Equip:"
    equipLabel->setText(qtTrId("equipview-equip"));
    equipBox = new QComboBox(this);
    equipBox->setObjectName("equipdef");

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
    layout->addWidget(searchLabel);
    layout->addWidget(searchBox);
    layout->addWidget(typeLabel);
    layout->addWidget(typeBox);
    layout->addWidget(equipLabel);
    layout->addWidget(equipBox);
    layout->addWidget(firstButton);
    layout->addWidget(prevButton);
    layout->addWidget(pageLabel);
    layout->addWidget(nextButton);
    layout->addWidget(lastButton);
    layout->addWidget(destructButton);
    layout->addWidget(addStarButton);


    QSizePolicy labelSize = QSizePolicy(QSizePolicy::Maximum,
                                        QSizePolicy::Preferred,
                                        QSizePolicy::Label);
    searchLabel->setSizePolicy(labelSize);
    QSizePolicy textEditSize = QSizePolicy(QSizePolicy::Maximum,
                                           QSizePolicy::Maximum,
                                           QSizePolicy::Label);
    searchBox->setSizePolicy(textEditSize);
    searchBox->setStyleSheet(QStringLiteral(
        "QLineEdit#searchbox { background-color: palette(button); }"
        ));
    //searchBox->setSizeAdjustPolicy(QLineEdit::AdjustToContents);
    //searchBox->setMaximumSize(QSize(50, 10));

    typeLabel->setSizePolicy(labelSize);
    typeBox->setSizePolicy(QSizePolicy(QSizePolicy::Maximum,
                                       QSizePolicy::Preferred,
                                       QSizePolicy::ComboBox));
    typeBox->resize(QSize(100, pageLabel->size().height()));
    //% "All equipments"
    typeBox->addItem(qtTrId("all-equipments"));
    typeBox->addItems(EquipType::getDisplayGroupsSorted());

    equipLabel->setSizePolicy(labelSize);
    QSizePolicy equipBoxSize = QSizePolicy(QSizePolicy::Maximum,
                                           QSizePolicy::Preferred,
                                           QSizePolicy::ComboBox);
    equipBox->setSizePolicy(equipBoxSize);
    equipBox->resize(QSize(100, pageLabel->size().height()));
    equipBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);

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

    connect(typeBox, &QComboBox::activated,
            model, &EquipModel::switchDisplayType);
    connect(equipBox, &QComboBox::activated,
            model, [this]{
                model->switchDisplayType2(equipBox->currentText());
            });
    connect(typeBox, &QComboBox::activated,
            this, &EquipView::reCalculateAvailableEquips);
    connect(destructButton, &QAbstractButton::clicked,
            model, &EquipModel::enactDestruct);

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
    equipBox->clear();
    for(auto &equipReg:
         Clientv2::getInstance().equipRegistryCache) {
        if(
            (typeBox->currentText().compare("All equipments") == 0
             && equipReg->type.getDisplayGroup()
                        .compare("VIRTUAL", Qt::CaseInsensitive) != 0
             && !equipReg->localNames.value("ja_JP").isEmpty())
            || equipReg->type.getDisplayGroup()
                       .compare(typeBox->currentText(),
                                Qt::CaseInsensitive) == 0) {
            QString equipName = equipReg->toString(
                settings->value("language", "ja_JP").toString());
            if(equipName.isEmpty()) {
                equipName = equipReg->toString("ja_JP");
            }
            equipBox->addItem(equipName);
        }
    }
}

void EquipView::pageNumChangedLambda(int current, int total) {
    columnResized(0, 0, 0);
    update();
}

void EquipView::activate(bool arsenal, bool isEquip) {
    disconnect(model, &EquipModel::pageNumChanged,
               this, &EquipView::pageNumChangedLambda);
    disconnect(searchBox, &QLineEdit::textEdited,
               model, &EquipModel::switchDisplayType2);
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
            destructButton->show();
            addStarButton->show();
        }
        else {
            model->setIsInArsenal(false);
            arsenalView->setItemDelegateForColumn(model->selectColumn(),
                                                  delegate);
            connect(delegate, &SelectDelegate::itemSelected,
                    this, &EquipView::itemSelected);
            destructButton->hide();
            addStarButton->hide();
        }
        recalculateArsenalRows();
        connect(model, SIGNAL(needReCalculateRows()),
                this, SLOT(recalculateArsenalRows()),
                Qt::UniqueConnection);
        connect(this, SIGNAL(rowCountHint(int)),
                model, SLOT(setRowsPerPageHint(int)),
                Qt::UniqueConnection);
        typeLabel->show();
        typeBox->show();
        equipLabel->show();
        equipBox->show();
    }
    else {;
        model = &engine.shipModel;
        arsenalView->setItemDelegateForColumn(model->hpColumn(), hpdelegate);;
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
            arsenalView->setItemDelegate(new QStyledItemDelegate());
            model->setIsInArsenal(true);
            addStarButton->show();
        }
        else {
            model->setIsInArsenal(false);
            arsenalView->setItemDelegateForColumn(model->selectColumn(),
                                                  delegate);
            connect(delegate, &SelectDelegate::itemSelected,
                    this, &EquipView::itemSelected);
            addStarButton->hide();
        }
        recalculateArsenalRows();
        connect(model, SIGNAL(needReCalculateRows()),
                this, SLOT(recalculateArsenalRows()),
                Qt::UniqueConnection);
        connect(this, SIGNAL(rowCountHint(int)),
                model, SLOT(setRowsPerPageHint(int)),
                Qt::UniqueConnection);
        typeLabel->hide();
        typeBox->hide();
        equipLabel->hide();
        equipBox->hide();
    }
    connect(model, &EquipModel::pageNumChanged,
            this, &EquipView::pageNumChangedLambda);
    connect(searchBox, &QLineEdit::textEdited,
            model, &EquipModel::switchDisplayType2);
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
