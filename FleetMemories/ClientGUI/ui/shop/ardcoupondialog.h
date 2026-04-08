/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef ARDCOUPONDIALOG_H
#define ARDCOUPONDIALOG_H

#include <QButtonGroup>
#include <QDialog>
#include <QLabel>
#include <QSpinBox>

/* Advances resource dispatch coupon */
class ARDCouponDialog : public QDialog {
    Q_OBJECT

public:
    explicit ARDCouponDialog(QWidget *parent = nullptr);

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void onPackageSelected(int id);
    void purchase();
    void updatePriceLabel(int units);

private:
    static constexpr int presetTiers[] = {100, 500, 2000, 10000, 50000};
    static constexpr int customTierId = 5;

    QButtonGroup *packageGroup;
    QSpinBox *unitsBox;
    QLabel *priceLabel;
};

#endif // ARDCOUPONDIALOG_H
