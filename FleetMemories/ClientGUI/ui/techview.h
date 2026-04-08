/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef TECHVIEW_H
#define TECHVIEW_H

#include <QFrame>
#include <QTableWidgetItem>
#include "tablewidgetitemnumber.h"

namespace Ui {
class TechView;
}

class TechView : public QFrame
{
    Q_OBJECT

public:
    explicit TechView(QWidget *parent = nullptr);
    ~TechView();

public slots:
    void demandGlobalTech();
    void demandLocalTech(int);
    void demandSkillPoints(int);
    void equipOrShip();
    void resetLocalListName();

signals:
    void skillPointInfo(int equipId, int skillPoint);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    void updateGlobalTech(const QJsonObject &);
    void updateGlobalTechViewTable(const QJsonObject &);
    void updateLocalTech(const QJsonObject &);
    void updateLocalTechViewTable(const QJsonObject &);
    void updateSkillPoints(const QJsonObject &);

private:
    void resizeColumns(bool);

    Ui::TechView *ui;
    bool isEquipChoice = true;
};



#endif // TECHVIEW_H
