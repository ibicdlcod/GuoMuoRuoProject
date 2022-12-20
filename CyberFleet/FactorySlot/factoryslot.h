#ifndef FACTORYSLOT_H
#define FACTORYSLOT_H

#include <QDateTime>
#include <QPushButton>
#include <QtUiPlugin/QDesignerExportWidget>

class QDESIGNER_WIDGET_EXPORT FactorySlot : public QPushButton
{
    Q_OBJECT

public:
    FactorySlot(QWidget *parent = 0);
    void setComplete(bool);
    void setCompleteTime(QDateTime);
    void setOpen(bool);
    void setSlotnum(int);
    void setStatus();

signals:
    void clickedSpec(bool checked = false, int slotnum = 0);

private slots:
    void clickedHelper(bool);

private:
    int slotnum;
    QDateTime completeTime = QDateTime();
    bool open = false;
    bool completed = false;
};

#endif // FACTORYSLOT_H
