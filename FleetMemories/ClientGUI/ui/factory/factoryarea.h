#ifndef FACTORYAREA_H
#define FACTORYAREA_H

#include <QFrame>
#include <QTableView>
#include <QHeaderView>
#include "developwindow.h"
#include "../../../FactorySlot/factoryslot.h"
#include "../views/equipview.h"
#include "../../../Protocol/kp.h"

namespace Ui {
class FactoryArea;
}


class FactoryArea : public QFrame
{
    Q_OBJECT

public:
    explicit FactoryArea(QWidget *parent = nullptr);
    ~FactoryArea();

    void setState(KP::FactoryState);
    void switchToState();

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void developClicked(bool checked = false, int slotnum = 0);
    void doDevelop(int);
    void doFactoryRefresh(const QJsonObject &);

private:
    Ui::FactoryArea *ui;
    EquipView *equipview;
    DevelopWindow w;

    KP::FactoryState factoryState = KP::Development;
    QList<FactorySlot *> slotfs;
    int currentSlotNum = 0;
};

#endif // FACTORYAREA_H
