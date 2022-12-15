#ifndef FACTORYSLOT_H
#define FACTORYSLOT_H

#include <QPushButton>
#include <QtUiPlugin/QDesignerExportWidget>

class QDESIGNER_WIDGET_EXPORT FactorySlot : public QPushButton
{
    Q_OBJECT

public:
    FactorySlot(QWidget *parent = 0);
    void setSlotnum(int);

signals:
    void clickedSpec(bool checked = false, int slotnum = 0);

private slots:
    void clickedHelper(bool);

private:
    int slotnum;
};

#endif // FACTORYSLOT_H
