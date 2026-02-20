/* Copyright (C) 2026 Harusoft Inc.
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

double setDropValue(double rarity,
                  std::mt19937 &engine) {
    static double a = 0.406226;
    static double b = 1.28317;
    std::exponential_distribution<> dist(a);
    double result;
    double test = dist(engine);
    result = test > b ? b : test;
    return result * rarity;
}


/* TBD: capital ships can nullify small damages */
double calDamage(double firepower, double armor, double armorPenetration,
                 std::mt19937 &engine,
                 bool overPenetrationEnabled = true) {
    double effectivePene = atan2(armorPenetration, armor);

    std::lognormal_distribution<> dist(effectivePene - atan(1),
                                       atan(1));
    double overPene = exp(3.0 * effectivePene - 4.0 * atan(1));
    double result = dist(engine);
    if(overPenetrationEnabled && result < overPene)
        return firepower * exp(-4.0 * atan(1));
    else
        return firepower * result;
}

int calDropCommon(const std::vector<ShipDropInfo> &shipInfo,
                  std::mt19937 &engine) {
    if(shipInfo.empty())
        return 0;
    std::vector<double> shipWeights;
    for(auto shipItem : shipInfo) {
        shipWeights.push_back(shipItem.weight);
    }
    std::discrete_distribution<> dist(shipWeights.begin(), shipWeights.end());
    return shipInfo[dist(engine)].shipDef;
}

/* Different from above is total weights may be < 100% and no drop as a result */
int calDropRare(const std::vector<ShipDropInfo> &shipInfo,
                std::mt19937 &engine) {
    if(shipInfo.empty())
        return 0;
    std::vector<double> shipWeights;
    for(auto shipItem : shipInfo) {
        shipWeights.push_back(shipItem.weight);
    }
    double weightSum = std::accumulate(shipWeights.begin(),
                                       shipWeights.end(),
                                       1.0,
                                       std::minus<double>());
    if(weightSum < 0.0)
        weightSum = 0.0;
    shipWeights.push_back(weightSum);
    std::discrete_distribution<> dist(shipWeights.begin(), shipWeights.end());
    int index = dist(engine);
    if(index == shipInfo.size())
        return 0;
    return shipInfo[index].shipDef;
}

}
#endif // RNGESUS_H
