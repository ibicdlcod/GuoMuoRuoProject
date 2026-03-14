/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef NAVIGATOR_H
#define NAVIGATOR_H

#include <QComboBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QToolButton>
#include "../model/equipmodel.h"

class Navi : public QObject {
    Q_OBJECT

public:
    Q_DECL_DEPRECATED explicit Navi(QHBoxLayout *layout, EquipModel *model);

    void enactPageNumChange(int currentPageNum, int totalPageNum);

private:
    QComboBox *typebox;
    QToolButton *firstbutton;
    QToolButton *prevbutton;
    QLabel *pageLabel;
    QToolButton *nextbutton;
    QToolButton *lastbutton;
    EquipModel *model;
};

#endif // NAVIGATOR_H
