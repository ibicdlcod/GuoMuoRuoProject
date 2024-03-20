#include "navigator.h"

Navi::Navi(QHBoxLayout *layout, EquipModel *model)
    : QObject(layout), model(model) {

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
            this, &Navi::enactPageNumChange);
}

void Navi::enactPageNumChange(int currentPageNum, int totalPageNum) {
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
    pageLabel->setText(QString::number(currentPageNum + 1)
                       + " / "
                       + QString::number(totalPageNum));
}
