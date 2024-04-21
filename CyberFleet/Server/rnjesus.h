#ifndef RNJESUS_H
#define RNJESUS_H

#include <numeric>
#include <random>

struct ShipDropInfo
{
    int shipDef;
    double weight;
};

namespace RNJesus {

/* TBD: capital ships can nullify small damages */
double calDamage(double firepower, double armor, double armorPenetration,
                 std::mt19937 &engine) {
    double effectivePene = armorPenetration / armor;
    static std::lognormal_distribution<> dist(0.5 * effectivePene - 1.0, effectivePene);
    return dist(engine) * firepower;
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

int calDropRare(const std::vector<ShipDropInfo> &shipInfo,
                std::mt19937 &engine) {
    if(shipInfo.empty())
        return 0;
    std::vector<double> shipWeights;
    for(auto shipItem : shipInfo) {
        shipWeights.push_back(shipItem.weight);
    }
    double weightSum = 1.0 - std::accumulate(shipWeights.begin(),
                                             shipWeights.end(),
                                             0.0);
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
#endif // RNJESUS_H
