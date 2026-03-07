/* Copyright (C) 2026 Harusoft Inc.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef SORTIE_H
#define SORTIE_H

#include <QFrame>
#include <QLabel>
#include "../../../Protocol/kp.h"
#include "maprender.h"
#include "mapdetail.h"
#include "mapviewwidget.h"
#include "opengl/battlewidget.h"

namespace Ui {
class Sortie;
}

class Sortie : public QFrame
{
    Q_OBJECT

public:
    explicit Sortie(QWidget *parent = nullptr);
    ~Sortie();

    void switchToState(KP::SortieState);
    KP::FleetType getCurrentFleetType();

public slots:
    void dealWithNode(const MapNode &node, int nodeId);

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void switchMap(int mapId);
    void confirmSortieStart();
    void sortieStart(const QJsonObject &djson);

private:
    Ui::Sortie *ui;
    MapRender *renderer;
    MapDetail *detail;
    BattleWidget *battleW;
    MapViewWidget *globeFrame;

    KP::SortieState sortieState = KP::MapView;
    int mapIndex = 0;
    QString mapStr;
    Map *currentMap;
};

#endif // SORTIE_H
