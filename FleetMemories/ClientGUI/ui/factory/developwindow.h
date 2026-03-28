/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef DEVELOPWINDOW_H
#define DEVELOPWINDOW_H

#include <QObject>
#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui { class DevelopWindow; }
QT_END_NAMESPACE

class DevelopWindow : public QDialog
{
    Q_OBJECT
public:
    explicit DevelopWindow(QWidget *parent = nullptr);
    ~DevelopWindow();

    int equipIdDesired();
    void displaySuccessRate2();

signals:
    void devDemandGlobalTech();
    void devDemandLocalTech(int);

public slots:
    void resetEquipName(int);
    void resetListName(int);
    void setBuyMode(bool);

private slots:
    void devDemandChance(bool checked = false);
    void displaySuccessRate(int);

private:
    Ui::DevelopWindow *ui;
    bool initial = true;
};

#endif // DEVELOPWINDOW_H
