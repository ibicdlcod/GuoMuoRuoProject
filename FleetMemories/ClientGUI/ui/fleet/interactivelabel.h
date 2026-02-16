/* Copyright (C) 2026 Harusoft Inc.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef INTERACTIVELABEL_H
#define INTERACTIVELABEL_H

#include <QLabel>
#include <QObject>
#include "fleetview.h"

class InteractiveLabel : public QLabel
{
    Q_OBJECT
public:
    explicit InteractiveLabel(int index = 0,
                              FleetView* parent = nullptr,
                              Qt::WindowFlags f = Qt::WindowFlags());
    void paintEvent(QPaintEvent *event) override;

public slots:
    void shipSelected(QUuid);
    void updateShipUId(QUuid);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    bool mousePressedInside = false;

    FleetView * parentView;
    QUuid shipUId;
    int index;
};

#endif // INTERACTIVELABEL_H
