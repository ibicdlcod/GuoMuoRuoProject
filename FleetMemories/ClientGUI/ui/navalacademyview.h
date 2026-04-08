/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef NAVALACADEMYVIEW_H
#define NAVALACADEMYVIEW_H

#include <QFrame>
#include <QTableWidgetItem>

#include "../Protocol/kp.h"

namespace Ui {
class NavalAcademyView;
}

class NavalAcademyView : public QFrame
{
    Q_OBJECT

public:
    explicit NavalAcademyView(QWidget *parent = nullptr);
    ~NavalAcademyView();

public slots:
    void demandLocalTech(int);
    void demandSkillPoints(int);
    void updateDstSkillPoints(const QJsonObject &);
    void updateSkillPointConvertResult(const QJsonObject &);
    void updateSrcSkillPoints(const QJsonObject &);

signals:
    void skillPointInfo(int equipId, int skillPoint);
    void convertSkillPoints(int srcEquipId, int dstEquipId, int64 amount);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    void onConvertClicked();
    void onDstEquipSelected(int index);
    void onSrcEquipSelected(int index);
    void updateAmountFromSlider(int value);
    void updateAmountFromSpinBox(int value);
    void updateLocalTech(const QJsonObject &);
    void updateLocalTechViewTable(const QJsonObject &);

private:
    void demandLocalTechForEquip(int equipId);
    void demandSkillPointsForEquip(int equipId);
    void filterDstEquipByMother(int motherId);
    void resetEquipmentLists();
    void resizeColumns(bool);
    void updateConvertButtonState();

    Ui::NavalAcademyView *ui;
    int currentSrcEquipId = 0;
    int currentDstEquipId = 0;
    int64 availableSkillPoints = 0;
};

class TableWidgetItemNumber: public QTableWidgetItem {
public:
    explicit TableWidgetItemNumber(double);
    virtual bool operator<(const QTableWidgetItem &other) const override {
        return this->text().toDouble() < other.text().toDouble();
    }
};

#endif // NAVALACADEMYVIEW_H