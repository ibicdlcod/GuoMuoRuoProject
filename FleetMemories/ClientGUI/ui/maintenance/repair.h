#ifndef REPAIR_H
#define REPAIR_H

#include <QWidget>

namespace Ui {
class Repair;
}

class Repair : public QWidget
{
    Q_OBJECT

public:
    explicit Repair(QWidget *parent = nullptr);
    ~Repair();

private:
    Ui::Repair *ui;
};

#endif // REPAIR_H
