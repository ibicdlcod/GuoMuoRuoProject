#ifndef FACTORYAREA_H
#define FACTORYAREA_H

#include <QFrame>
#include <QTableView>
#include "FactorySlot/factoryslot.h"
#include "../../Protocol/kp.h"

namespace Ui {
class FactoryArea;
}

class FactoryArea : public QFrame
{
    Q_OBJECT

public:
    explicit FactoryArea(QWidget *parent = nullptr);
    ~FactoryArea();

    void setDevelop(KP::FactoryState);
    void switchToDevelop();

private slots:
    void developClicked(bool checked = false, int slotnum = 0);
    void doFactoryRefresh(const QJsonObject &);
    void updateArsenalEquip(const QJsonObject &);

private:
    Ui::FactoryArea *ui;
    QTableView *arsenalView;

    KP::FactoryState factoryState = KP::Development;
    QList<FactorySlot *> slotfs;
};

#endif // FACTORYAREA_H
