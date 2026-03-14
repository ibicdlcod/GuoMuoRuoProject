/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef EQUIPSELECT_H
#define EQUIPSELECT_H

#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>

class EquipSelect : public QWidget
{
    Q_OBJECT
public:
    explicit EquipSelect(int height, QWidget *parent = nullptr);

signals:
    void typeActivated(int);
    void equipActivated(const QString &);
    void destructActivated(bool checked = false);
    void searchBoxChanged(const QString &);

public slots:
    void reCalculateAvailableEquips(int);

public:
    QLabel *searchLabel;
    QLineEdit *searchBox;
    QLabel *typeLabel;
    QComboBox *typeBox;
    QLabel *equipLabel;
    QComboBox *equipBox;

    QPushButton *destructButton;
    QPushButton *addStarButton;

private:
    int height;
};

#endif // EQUIPSELECT_H
