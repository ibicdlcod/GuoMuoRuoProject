/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef CONSTRUCTWINDOW_H
#define CONSTRUCTWINDOW_H

#include <QDialog>
#include <QComboBox>
#include "../../model/shipdefmodel.h"

namespace Ui {
class ConstructWindow;
}

class ConstructWindow : public QDialog
{
    Q_OBJECT

public:
    explicit ConstructWindow(QWidget *parent = nullptr);
    ~ConstructWindow();

    void initialize();
    void setCloningMode(bool);
    int shipDefDesired();
    QList<QUuid> defaultEquipsDesired();
    QUuid shipToRemodelDesired();
    void updateSanityDisplay();

public slots:
    void switchDisplay(int dummy = 0);
    void shipNameChanged(int dummy = 0);

private:
    Ui::ConstructWindow *ui;
    QList<QComboBox *> equipBoxes;

    bool cloningMode = false;
    int shipDef;
    double sanityRemaining = 0.0;
    double sanityRequired = 0.0;
};

#endif // CONSTRUCTWINDOW_H
