/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef SEGMENTEDHPBAR_H
#define SEGMENTEDHPBAR_H

#include <QWidget>

class QPaintEvent;

class SegmentedHPBar : public QWidget
{
    Q_OBJECT

public:
    explicit SegmentedHPBar(QWidget *parent = nullptr);

    void setValues(int totalHP, int previousHP, int currentHP);
    void setInverted(bool inverted);
    void setFled(bool fled);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    int m_totalHP = 1;
    int m_previousHP = 1;
    int m_currentHP = 1;
    bool m_inverted = false;
    bool m_fled = false;
};

#endif // SEGMENTEDHPBAR_H