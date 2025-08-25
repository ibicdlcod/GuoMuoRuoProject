#ifndef MAPRENDER_H
#define MAPRENDER_H

#include <QWidget>
#include <QPen>
#include "../../Protocol/kp.h"

class MapRender : public QWidget
{
    Q_OBJECT
public:
    explicit MapRender(QWidget *parent = nullptr);

#pragma message(NOT_M_CONST)
    static constexpr int globeMapWidth = 5632;
    static constexpr int globeMapHeight = 2048;

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    QPen pen;
    QBrush brush;
    bool antialiased;
    QPixmap pixmap;

    bool mousePressedInside = false;
};

#endif // MAPRENDER_H
