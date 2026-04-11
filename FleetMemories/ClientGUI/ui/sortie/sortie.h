/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef SORTIE_H
#define SORTIE_H

#include <QFrame>
#include <QLabel>
#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <QByteArray>
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

class QGroupBox;
class QPushButton;
class QSlider;
class QCheckBox;

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
    void disasterLOSInfo(const QJsonObject &djson);
    void transportFreightInfo(const QJsonObject &djson);
    void recalculateAttrition();
    void sortieEnd();
    void sortieStart(const QJsonObject &djson);
    void switchMap(int mapId);
    void expeditionNodeClicked(int nodeId);
    // Expedition slots
    void expeditionStartResult(int mapUnionId, bool accepted, KP::GameError error);
    void expeditionStatus(const QJsonArray &expeditions);
    void expeditionProgressUpdate(int mapUnionId, int nodeIndex, const QJsonObject &battleResult);
    void expeditionStopped(int mapUnionId, int stopReason);
    void startExpedition();
    void cancelExpedition();
    void updateExpeditionSettings();
    void updateExpeditionAutoRestart();
    void updateAutoRestartLabel();
    void saveExpeditionSettings();
    void planExpeditionNodes();

private:
    Ui::Sortie *ui;
    MapRender *renderer;
    MapDetail *detail;
    BattleWidget *battleW;
    ResourceGainView *resourceGainW;
    MapViewWidget *globeFrame;
    // Expedition UI
    QGroupBox *expeditionGroup;
    QPushButton *expeditionPlanButton;
    QPushButton *expeditionStartButton;
    QPushButton *expeditionCancelButton;
    QSlider *thresholdSlider;
    QLabel *thresholdLabel;
    QCheckBox *autoRestartCheckBox;
    QPushButton *saveSettingsButton;

    KP::SortieState sortieState = KP::MapView;
    int mapIndex = 0;
    QString mapStr;
    MapWithDiff *currentMap = nullptr;
    int currentNodeId = 0;
    QJsonObject currentBattleProcess;

    // Expedition state
    bool expeditionMode = false;
    QMap<int, QMap<int, QByteArray>> expeditionBattlePlans;
    double autoRestartThreshold = 1.0; // 100%
    bool autoResupply = true;
    int expeditionFleetIndex = 0;
};

#endif // SORTIE_H
