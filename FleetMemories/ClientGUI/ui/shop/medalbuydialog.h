/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef MEDALBUYDIALOG_H
#define MEDALBUYDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QSpinBox>

class MedalBuyDialog : public QDialog {
    Q_OBJECT

public:
    explicit MedalBuyDialog(QWidget *parent = nullptr);

private slots:
    void onAmountChanged(int amount);
    void purchase();

private:
    QSpinBox *amountBox;
    QLabel *costLabel;
};

#endif // MEDALBUYDIALOG_H
