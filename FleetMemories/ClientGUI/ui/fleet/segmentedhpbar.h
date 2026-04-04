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

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    int m_totalHP = 1;
    int m_previousHP = 1;
    int m_currentHP = 1;
};

#endif // SEGMENTEDHPBAR_H