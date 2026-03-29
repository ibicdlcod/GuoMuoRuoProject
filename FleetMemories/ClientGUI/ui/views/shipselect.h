/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef SHIPSELECT_H
#define SHIPSELECT_H

#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>

class ShipSelect : public QWidget
{
    Q_OBJECT
public:
    explicit ShipSelect(int height, QWidget *parent = nullptr);

signals:
    void decorateActivated(bool checked = false);
    void modernizeActivated(bool checked = false);
    void selectChanged(const QString &nationality,
                       const QString &shiptype,
                       const QString &shipclass,
                       const QString &searchTerm
                       = QLatin1String(""));
    void supplyActivated(bool checked = false);
    void supplyAllActivated(bool checked = false);

public slots:
    void typeBoxHinted(QStringList &types);
    void classBoxHinted(QStringList &types);

public:
    QLabel *searchLabel;
    QLineEdit *searchBox;
    QLabel *nationLabel;
    QComboBox *nationBox;
    QLabel *typeLabel;
    QComboBox *typeBox;
    QLabel *classLabel;
    QComboBox *classBox;

    QPushButton *addStarButton;
    QPushButton *decorateButton;
    QPushButton *supplyAllButton;
    QPushButton *supplyButton;

private:
    int height;
};

#endif // SHIPSELECT_H
