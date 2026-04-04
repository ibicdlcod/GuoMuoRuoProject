/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef BATTLERESULTDIALOG_H
#define BATTLERESULTDIALOG_H

#include <QDialog>
#include <QJsonObject>

namespace Ui {
class BattleResultDialog;
}

class BattleResultDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BattleResultDialog(QWidget *parent = nullptr);
    ~BattleResultDialog();

    void populate(const QJsonObject &battleProcess);

private:
    Ui::BattleResultDialog *ui;
};

#endif // BATTLERESULTDIALOG_H
