/* Copyright (C) 2026 Harusoft Inc.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef SHIPTYPE_H
#define SHIPTYPE_H

#include <QHash>
#include <QString>
#include "resord.h"

class ShipType
{
public:
    ShipType(int shipId);

    bool operator==(const ShipType &) const;

    const ResOrd consResBase() const;
    int consTimeBase() const;
    QString toString() const;
    int toInt() const;
    QString iconGroup() const;
    int getTypeSort() const;
    int getCapitalness() const;
    KP::CapitalType getCapitalType() const;

private:
    int iRep;
};

#endif // SHIPTYPE_H
