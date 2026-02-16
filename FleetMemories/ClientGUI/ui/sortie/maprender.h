/* Copyright (C) 2026 Harusoft Inc.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef MAPRENDER_H
#define MAPRENDER_H

#include <QWidget>
#include <QPen>
#include "../../../Protocol/kp.h"

class MapRender : public QWidget
{
    Q_OBJECT
public:
    explicit MapRender(QWidget *parent = nullptr);

#pragma message(NOT_M_CONST)
    static constexpr int globeMapWidth = 5632;
    static constexpr int globeMapHeight = 2048;
    static constexpr int circleSize = 64;

signals:
    void mapSelected(int mapId);

protected:
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    QPen pen;
    QBrush brush;
    QBrush brushHovered;
    bool antialiased;
    QPixmap pixmap;

    bool mousePressedInside = false;
    int hoverMapID = 0;
};

#endif // MAPRENDER_H
