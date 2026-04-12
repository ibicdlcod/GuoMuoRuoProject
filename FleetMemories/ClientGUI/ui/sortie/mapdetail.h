/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef MAPDETAIL_H
#define MAPDETAIL_H

#include <QWidget>
#include <QPropertyAnimation>
#include <QSet>
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
    static constexpr int circleBorderSize = 4;
    static constexpr int circleSize = 24;

    Q_PROPERTY(QPointF fleetcenter
                   READ getFleetCenter
                       WRITE setFleetCenter
                           NOTIFY fleetCenterChanged)
    void changeCurrentNode(const MapNode &node);
    void displayDetailedMap(Map *map);
    QPointF getFleetCenter() const;
    static QPixmap recolorImage(const QString &filename, const QColor &color);
    void setChoiceNodes(const QList<int> &nodeIds);
    void setExpeditionMode(bool expedition);
    void setPlannedNodes(const QSet<int> &nodeIds);
    void setFleetCenter(const QPointF &input);

signals:
    void fleetCenterChanged();
    void moveFinished();
    void nodeClicked(int nodeId);

private:
    virtual void paintEvent(QPaintEvent *event) override;
    virtual void mousePressEvent(QMouseEvent *event) override;

private:
    Ui::MapDetail *ui;

    QPropertyAnimation *animation;
    Map *mapPointer;
    bool antialiased;
    QPointF fleetCenter;
    MapNode currentNode;
    bool uninitialized = true;
    KP::FleetType currentFleetType;

    QList<int> choiceNodeIds;
    bool awaitingChoice = false;
    bool expeditionMode = false;
    QSet<int> plannedNodeIds;

    QPixmap rudder;
    QPixmap airNodeIcon;
    QPixmap carrierFleetIcon;
    QPixmap surfaceFleetIcon;
    QPixmap transportFleetIcon;
    QPixmap normalFleetIcon;
};

#endif // MAPDETAIL_H
