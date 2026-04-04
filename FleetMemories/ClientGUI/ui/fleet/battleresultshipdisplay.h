/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef BATTLERESULTSHIPDISPLAY_H
#define BATTLERESULTSHIPDISPLAY_H

#include <QString>
#include <QVector>
#include <QWidget>

class SegmentedHPBar;

class BattleResultShipDisplay : public QWidget
{
    Q_OBJECT

public:
    explicit BattleResultShipDisplay(QWidget *parent = nullptr,
                                     int shipIndex = 0,
                                     const QString &shipName = QString(),
                                     int hpBefore = 1,
                                     int hpAfter = 1,
                                     int totalHP = 1,
                                     const QVector<int> &planeLosses = QVector<int>());

private slots:
    void showPlaneLosses();

private:
    int m_shipIndex;
    QString m_shipName;
    int m_hpBefore;
    int m_hpAfter;
    int m_totalHP;
    QVector<int> m_planeLosses;
    SegmentedHPBar *m_hpBar;
};

#endif // BATTLERESULTSHIPDISPLAY_H