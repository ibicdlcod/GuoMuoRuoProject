/* Copyright (C) 2026 Harusoft Ltd.
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
    int getCapitalness() const;
    KP::CapitalType getCapitalType() const;
    int getTypeSort() const;
    QString iconGroup() const;
    const ResOrd repairResBase() const;
    double repairTimeBase() const;
    QString toDetailedString() const;
    int toInt() const;
    QString toString() const;

private:
    int iRep;
};

#endif // SHIPTYPE_H
