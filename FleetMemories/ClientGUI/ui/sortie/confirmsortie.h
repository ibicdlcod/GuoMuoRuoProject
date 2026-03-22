/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef CONFIRMSORTIE_H
#define CONFIRMSORTIE_H

#include <QDialog>
#include <QObject>
#include "ui_confirmsortie.h"
#include <QString>
#include "../fleet/fleetview.h"
#include "sortie.h"

class ConfirmSortie : public QDialog
{
    Q_OBJECT

public:
    explicit ConfirmSortie(QWidget *parent = nullptr,
                           QString mapText = QStringLiteral(""),
                           QString diffText = QStringLiteral(""));
    ~ConfirmSortie();

    int getFleetIndex() const;
    friend void Sortie::battleEnd();

private:
    Ui::ConfirmSortie *ui;
    FleetView *fv;
};

#endif // CONFIRMSORTIE_H
