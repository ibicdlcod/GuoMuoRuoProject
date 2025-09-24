#ifndef SHIPDISPLAY_H
#define SHIPDISPLAY_H

#include <QWidget>

namespace Ui {
class ShipDisplay;
}

class ShipDisplay : public QWidget
{
    Q_OBJECT

public:
    explicit ShipDisplay(QWidget *parent = nullptr);
    ~ShipDisplay();

    void setContent(int currentHP, int maxHP, int cond, int lv);

private:
    Ui::ShipDisplay *ui;
};

#endif // SHIPDISPLAY_H
