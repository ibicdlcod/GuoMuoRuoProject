/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef EQUIPVIEW_H
#define EQUIPVIEW_H

#include <QCheckBox>
#include <QComboBox>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableView>
#include <QTimer>
#include <QToolButton>
#include <QWidget>
#include <QLineEdit>
#include <QButtonGroup>
#include <QRadioButton>
#include <QStackedLayout>
#include "../../model/equipmodel.h"
#include "selectdelegate.h"
#include "hpdelegate.h"
#include "equipselect.h"
#include "shipselect.h"
#include "industrialselect.h"

namespace Ui {
class EquipView;
}

namespace {
/* source: https://stackoverflow.com/questions/8766633/
 * how-to-determine-the-correct-size-of-a-qtablewidget */
static QSize tableSizeWhole(QTableView *view, EquipModel *model) {
    int w = view->verticalHeader()->width() + 4; // +4 seems to be needed
    for (int i = 0; i < model->columnCount(); i++) {
        w += view->columnWidth(i); // seems to include gridline (on my machine)
    }
    int h = view->horizontalHeader()->height() + 4;
    for (int i = 0; i < model->rowCount(); i++)
        h += view->rowHeight(i);
    return QSize(w, h);
}
}

class EquipView : public QWidget
{
    Q_OBJECT

public:
    explicit EquipView(QWidget *parent = nullptr);
    ~EquipView();

    void activate(bool arsenal = true, bool isEquip = true,
                  std::optional<KP::FactoryState> custom = std::nullopt);
    void enactPageNumChange(int currentPageNum, int totalPageNum);

    int getRowCountHintVal(){ return rowCountHintVal; }

public slots:
    void recalculateArsenalRows();
    void pageNumChangedLambda(int, int);

signals:
    void rowCountHint(int);
    void equipSelected(QUuid id);
    void shipSelected(QUuid id);
    void buyActivated(bool checked = false);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private slots:
    void columnResized(int logicalIndex, int oldSize, int newSize);
    void itemSelected(QUuid id);

private:
    Ui::EquipView *ui;

    EquipModel *model;
    QTableView *arsenalView;
    SelectDelegate *delegate;
    HpDelegate *hpdelegate;

    /* shared */
    QComboBox *sortBox;
    QCheckBox *reverseCheck;
    QToolButton *firstButton;
    QToolButton *prevButton;
    QLabel *pageLabel;
    QToolButton *nextButton;
    QToolButton *lastButton;
    QPushButton *unselectButton;

    EquipSelect *equipSelect;
    ShipSelect *shipSelect;
    IndustrialSelect *industrialSelect;
    QStackedLayout *lay;

    QButtonGroup *fleetFilterGroup;
    QRadioButton *fleetRadioAll;
    QRadioButton *fleetRadio1;
    QRadioButton *fleetRadio2;
    QRadioButton *fleetRadio3;
    QRadioButton *fleetRadio4;
    QRadioButton *fleetRadioUnassigned;

    int rowCountHintVal = 1;
    QTimer *columnResizeDebounce;
};

#endif // EQUIPVIEW_H
