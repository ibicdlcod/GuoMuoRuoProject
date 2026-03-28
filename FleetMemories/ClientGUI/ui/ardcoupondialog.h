/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef ARDCOUPONDIALOG_H
#define ARDCOUPONDIALOG_H

#include <QButtonGroup>
#include <QDialog>

/* Advances resource dispatch coupon */
class ARDCouponDialog : public QDialog {
    Q_OBJECT

public:
    explicit ARDCouponDialog(QWidget *parent = nullptr);

private slots:
    void purchase();

private:
    static double realPrice(double price);

    QButtonGroup *packageGroup;
};

#endif // ARDCOUPONDIALOG_H
