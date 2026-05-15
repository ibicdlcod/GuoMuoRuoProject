/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef CONFIRMSORTIE_H
#define CONFIRMSORTIE_H

#include "ui_confirmsortie.h"

#include <QDialog>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QWidget>
#include <QLabel>

class QSplitter;
class QGridLayout;
class QLabel;
class QPushButton;
class QDialog;

#include "../fleet/fleetview.h"
#include "sortie.h"

class BattleDetailDialog : public QDialog
{
    Q_OBJECT
public:
    explicit BattleDetailDialog(const QJsonArray &damageLog,
                                FleetView *fv,
                                const QJsonArray &enemyShipIds,
                                QWidget *parent = nullptr);
};

class ConfirmSortie : public QDialog
{
    Q_OBJECT

public:
    explicit ConfirmSortie(QWidget *parent = nullptr,
                           QString mapText = QStringLiteral(""),
                           QString diffText = QStringLiteral(""));
    ~ConfirmSortie();

    int getFleetIndex() const;
    void showBattleResult(const QJsonObject &battleProcess);
    friend void Sortie::battleEnd();

private:
    void clearBattleResultLayout();
    void populateBattleResult(const QJsonObject &battleProcess);
    void createBattleResultLayout();
    void showBattleDetail();

private:
    Ui::ConfirmSortie *ui;
    FleetView *fv;
    bool m_battleResultMode;
    QJsonObject m_battleProcess;
    QJsonArray m_damageLog;
    QJsonArray m_enemyShipIds;
    QSplitter *m_battleSplitter;
    QWidget *m_playerContainer;
    QWidget *m_enemyContainer;
    QGridLayout *m_playerLayout;
    QGridLayout *m_enemyLayout;
    QLabel *m_assessmentLabel;
    QPushButton *m_detailButton;
};

#endif // CONFIRMSORTIE_H
