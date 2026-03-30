/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef BUYORDRESOURCESDIALOG_H
#define BUYORDRESOURCESDIALOG_H

#include <QButtonGroup>
#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>

class BuyOrdResourcesDialog : public QDialog {
    Q_OBJECT

public:
    explicit BuyOrdResourcesDialog(QWidget *parent = nullptr);

private slots:
    void purchase();
    void updatePreview();

private:
    QButtonGroup *resourceGroup;
    QSpinBox *couponsBox;
    QLabel *rateLabel;
    QLabel *receiveLabel;
    QPushButton *buyBtn;
};

#endif // BUYORDRESOURCESDIALOG_H
