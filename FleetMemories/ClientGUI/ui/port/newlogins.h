/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef NewLoginS_H
#define NewLoginS_H

#include <QFrame>

namespace Ui {
class NewLoginS;
}

class NewLoginS : public QFrame
{
    Q_OBJECT

public:
    explicit NewLoginS(QWidget *parent = nullptr);
    ~NewLoginS();

private slots:
    void parseConnectReq();

private:
    Ui::NewLoginS *ui;
};

#endif // NewLoginS_H
