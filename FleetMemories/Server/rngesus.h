/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef RNGESUS_H
#define RNGESUS_H

#include <numeric>
#include <random>

struct ShipDropInfo
{
    int shipDef;
    double weight;
};

namespace RNGesus {

double dropValue(double rarity,
                          std::mt19937 &engine);

/* TBD: capital ships can nullify small damages */
double calDamage(double firepower, double armor, double armorPenetration,
                          std::mt19937 &engine,
                          bool overPenetrationEnabled = true);

int calDropCommon(const std::vector<ShipDropInfo> &shipInfo,
                           std::mt19937 &engine);

int calDropRare(const std::vector<ShipDropInfo> &shipInfo,
                         std::mt19937 &engine);

}
#endif // RNGESUS_H
