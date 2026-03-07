#ifndef MAPDETAIL_H
#define MAPDETAIL_H

#include <QWidget>
#include "../../Protocol/map.h"

namespace Ui {
class MapDetail;
}

class MapDetail : public QWidget
{
    Q_OBJECT

public:
    explicit MapDetail(QWidget *parent = nullptr);
    ~MapDetail();
    static constexpr int circleBorderSize = 2;
    static constexpr int circleSize = 24;

    void displayDetailedMap(Map *map);

private:
    virtual void paintEvent(QPaintEvent *event) override;

private:
    Ui::MapDetail *ui;

    Map *mapPointer;
    bool antialiased;
    QPixmap rudder;
};

#endif // MAPDETAIL_H
