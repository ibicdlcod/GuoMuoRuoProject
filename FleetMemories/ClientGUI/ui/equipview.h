#ifndef EQUIPVIEW_H
#define EQUIPVIEW_H

#include <QComboBox>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableView>
#include <QToolButton>
#include <QWidget>
#include <QLineEdit>
#include "../equipmodel.h"
#include "selectdelegate.h"
#include "hpdelegate.h"

namespace Ui {
class EquipView;
}

namespace {
/* source: https://stackoverflow.com/questions/8766633/
 * how-to-determine-the-correct-size-of-a-qtablewidget */
static QSize tableSizeWhole(QTableView *view, EquipModel *model) {
    int w = view->verticalHeader()->width() + 4; // +4 seems to be needed
    for (int i = 0; i < model->columnCount(); i++)
        w += view->columnWidth(i); // seems to include gridline (on my machine)
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

    void activate(bool arsenal = true, bool isEquip = true);
    void enactPageNumChange(int currentPageNum, int totalPageNum);

public slots:
    void recalculateArsenalRows();
    void reCalculateAvailableEquips(int);
    void pageNumChangedLambda(int, int);

signals:
    void rowCountHint(int);

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void columnResized(int logicalIndex, int oldSize, int newSize);
    void itemSelected(QUuid id);

private:
    Ui::EquipView *ui;

    EquipModel *model;
    QTableView *arsenalView;
    SelectDelegate *delegate;
    HpDelegate *hpdelegate;

    QLabel *searchLabel;
    QLineEdit *searchBox;
    QLabel *typeLabel;
    QComboBox *typeBox;
    QLabel *equipLabel;
    QComboBox *equipBox;
    QToolButton *firstButton;
    QToolButton *prevButton;
    QLabel *pageLabel;
    QToolButton *nextButton;
    QToolButton *lastButton;
    QPushButton *destructButton;
    QPushButton *addStarButton;
};

#endif // EQUIPVIEW_H
