/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef BUYEQUIPDIALOG_H
#define BUYEQUIPDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>

class BuyEquipDialog : public QDialog {
    Q_OBJECT

public:
    explicit BuyEquipDialog(QWidget *parent = nullptr);

private slots:
    void purchase();
    void selectionChanged();

private:
    QListWidget *equipList;
    QLabel *priceLabel;
    QPushButton *buyBtn;
};

#endif // BUYEQUIPDIALOG_H
