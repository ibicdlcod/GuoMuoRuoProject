/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef CHOOSEHOMEPORT_H
#define CHOOSEHOMEPORT_H

#include <QDialog>
#include "../../../Protocol/kp.h"

namespace Ui {
class ChooseHomePort;
}

class ChooseHomePort : public QDialog
{
    Q_OBJECT

public:
    explicit ChooseHomePort(QWidget *parent = nullptr,
                            const QJsonObject input = QJsonObject());
    ~ChooseHomePort();

signals:
    void portChosen(KP::AllegianceGroup);

public slots:
    void finishChoice(int status);

private:
    Ui::ChooseHomePort *ui;
};

#endif // CHOOSEHOMEPORT_H
