#ifndef FACTORYAREA_H
#define FACTORYAREA_H

#include <QFrame>

namespace Ui {
class FactoryArea;
}

class FactoryArea : public QFrame
{
    Q_OBJECT

public:
    explicit FactoryArea(QWidget *parent = nullptr);
    ~FactoryArea();

private:
    Ui::FactoryArea *ui;
};

#endif // FACTORYAREA_H
