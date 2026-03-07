#ifndef BATTLEPLAN_H
#define BATTLEPLAN_H

#include <QDialog>

namespace Ui {
class BattlePlan;
}

class BattlePlan : public QDialog
{
    Q_OBJECT

public:
    explicit BattlePlan(QWidget *parent = nullptr);
    ~BattlePlan();

private:
    Ui::BattlePlan *ui;
};

#endif // BATTLEPLAN_H
