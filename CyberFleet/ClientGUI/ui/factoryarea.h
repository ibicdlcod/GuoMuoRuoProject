#ifndef FACTORYAREA_H
#define FACTORYAREA_H

#include <QFrame>
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

    void setDevelop(bool);
    void switchToDevelop();

private slots:
    void developClicked(bool checked = false, int slotnum = 0);
    void doFactoryRefresh(const QJsonObject &);

private:
    Ui::FactoryArea *ui;

    KP::FactoryState factoryState = KP::Development;
    QList<FactorySlot *> slotfs;
};

#endif // FACTORYAREA_H
