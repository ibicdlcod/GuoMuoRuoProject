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
class QScrollArea;
class QGridLayout;
class QLabel;

#include "../fleet/fleetview.h"
#include "sortie.h"

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

private:
    Ui::ConfirmSortie *ui;
    FleetView *fv;
    bool m_battleResultMode;
    QJsonObject m_battleProcess;
    QSplitter *m_battleSplitter;
    QScrollArea *m_playerScroll;
    QScrollArea *m_enemyScroll;
    QWidget *m_playerContainer;
    QWidget *m_enemyContainer;
    QGridLayout *m_playerLayout;
    QGridLayout *m_enemyLayout;
    QLabel *m_assessmentLabel;
};

#endif // CONFIRMSORTIE_H
