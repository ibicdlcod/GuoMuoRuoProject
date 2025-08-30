#ifndef FACTORYSLOT_H
#define FACTORYSLOT_H

#include <QDateTime>
#include <QPushButton>
#include <QtUiPlugin/QDesignerExportWidget>
#include <QTimer>

class QDESIGNER_WIDGET_EXPORT FactorySlot : public QPushButton
{
    Q_OBJECT

public:
    FactorySlot(QWidget *parent = 0);
    ~FactorySlot() noexcept;
    bool isOpen();
    bool isComplete();
    bool isOnJob();
    void setComplete(bool);
    void setCompleteTime(QDateTime);
    void setOpen(bool);
    void setSlotnum(int);

public slots:
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
    QTimer *timer;
};

#endif // FACTORYSLOT_H
