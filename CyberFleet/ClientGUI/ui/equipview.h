#ifndef EQUIPVIEW_H
#define EQUIPVIEW_H

#include <QComboBox>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableView>
#include <QWidget>
#include "../equipmodel.h"

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

    void activate(bool arsenal = true);
    void enactPageNumChange(int currentPageNum, int totalPageNum);

public slots:
    void recalculateArsenalRows();

signals:
    void rowCountHint(int);

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void columnResized(int logicalIndex, int oldSize, int newSize);

private:
    Ui::EquipView *ui;

    EquipModel *model;
    QTableView *arsenalView;

    QComboBox *typebox;
    QToolButton *firstButton;
    QToolButton *prevButton;
    QLabel *pageLabel;
    QToolButton *nextButton;
    QToolButton *lastButton;
    QPushButton *destructButton;
    QPushButton *addStarButton;
};

#endif // EQUIPVIEW_H
