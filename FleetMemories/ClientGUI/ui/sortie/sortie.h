/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef SORTIE_H
#define SORTIE_H

#include <QFrame>
#include <QLabel>
#include <QJsonObject>
#include "../../../Protocol/kp.h"
#include "mapdetail.h"
#include "maprender.h"
#include "mapviewwidget.h"
#include "opengl/battlewidget.h"
#include "resourcegainview.h"
#include "../../../Protocol/mapwithdiff.h"

namespace Ui {
class Sortie;
}

class Sortie : public QFrame
{
    Q_OBJECT

public:
    explicit Sortie(QWidget *parent = nullptr);
    ~Sortie();

    KP::FleetType getCurrentFleetType();
    void switchToState(KP::SortieState);

public slots:
    void battleEnd();
    void dealWithNode(const MapNode &node, int nodeId);

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void battleProcess(const QJsonObject &djson);
    void confirmSortieStart();
    void recalculateAttrition();
    void sortieEnd();
    void sortieStart(const QJsonObject &djson);
    void switchMap(int mapId);

private:
    Ui::Sortie *ui;
    MapRender *renderer;
    MapDetail *detail;
    BattleWidget *battleW;
    ResourceGainView *resourceGainW;
    MapViewWidget *globeFrame;

    KP::SortieState sortieState = KP::MapView;
    int mapIndex = 0;
    QString mapStr;
    MapWithDiff *currentMap;
    int currentNodeId = 0;
    QJsonObject currentBattleProcess;
};

#endif // SORTIE_H
