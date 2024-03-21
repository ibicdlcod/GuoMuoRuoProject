#ifndef FACTORYAREA_H
#define FACTORYAREA_H

#include <QFrame>
#include <QTableView>
#include <QHeaderView>
#include "FactorySlot/factoryslot.h"
#include "equipview.h"
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

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void developClicked(bool checked = false, int slotnum = 0);
    void doFactoryRefresh(const QJsonObject &);

private:
    Ui::FactoryArea *ui;
    EquipView *equipview;

    KP::FactoryState factoryState = KP::Development;
    QList<FactorySlot *> slotfs;
};

#endif // FACTORYAREA_H
