#ifndef PORTAREA_H
#define PORTAREA_H

#include <QWidget>

namespace Ui {
class PortArea;
}

class PortArea : public QWidget
{
    Q_OBJECT

public:
    explicit PortArea(QWidget *parent = nullptr);
    ~PortArea();

private:
    Ui::PortArea *ui;
};

#endif // PORTAREA_H
