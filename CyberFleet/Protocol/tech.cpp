#include "tech.h"
#include <QDebug>
#include <QSettings>
#include "kp.h"

extern std::unique_ptr<QSettings> settings;

/* 2-Technology.md#Combined effects */
double Tech::calCapable(double globalTech, double localTech,
                        double wantedTech) {
    double maxDeterimental =
        settings->value("rule/techcombinedeffects", 3.0).toDouble();
    double min = std::min(globalTech, localTech);
    double max = std::max(globalTech, localTech);
    double detrimentalEffect = 0.0;
    if(min < wantedTech) {
        double lag = wantedTech - min;
        detrimentalEffect = maxDeterimental * lag / std::hypot(1.0, lag);
    }
    return std::max(min, max - detrimentalEffect);
}

/* 2-Technology.md#Success rate */
bool Tech::calExperiment(const double wantedTech,
                         const double currentTech,
                         const double sigmaConstant,
                         std::mt19937 &generator) {
    assert(sigmaConstant > 0.0);
    std::normal_distribution d{wantedTech, sigmaConstant};
    double random_double = d(generator);
    return random_double <= currentTech;
}

/* 2-Technology.md#Success rate */
bool Tech::calExperiment2(const double wantedTech,
                          const double globalTech,
                          const double localTech,
                          const double sigmaConstant,
                          std::mt19937 &generator) {
    return calExperiment(wantedTech,
                         calCapable(globalTech, localTech, wantedTech),
                         sigmaConstant,
                         generator);
}

/* 2-Technology.md#How a combined level is calculated from its components */
double Tech::calLevel(QList<std::pair<double, double>> &source,
                      const double scopeConstant) {
    /* sort by key desc */
    std::sort(source.begin(), source.end(),
              [](std::pair<double, double> a, std::pair<double, double> b){
                  return a.first > b.first;
              });
    if(scopeConstant <= 1.0) {
        //% "Scope constant less or equal to 1 is against design doctrine!"
        qCritical() << qtTrId("scope-constant-less-than-1");
        return 0.0;
    }
    double result = 0.0;
    double weightSum = 0.0;
    for(auto iter = source.begin();
         iter != source.end();
         iter++) {
        if(iter->first < 0.0) {
            //% "Tech level less than 0 is against design doctrine!"
            qCritical() << qtTrId("tech-level-less-than-0");
            return 0.0;
        }
        double weight = iter->second;
        if(weight < 0.0) {
            weight = 0.0;
        }
        weightSum += weight;
        result += (iter->first) * (pow(scopeConstant, weight) - 1.0)
                  * (pow(scopeConstant, -weightSum));
    }
    return result;
}

/* 2-Technology.md#Constants */
double Tech::calLevelGlobal(QList<std::pair<double, double>> &source) {
    double globalScope =
        settings->value("rule/techglobalscope", 1.02).toDouble();
    return calLevel(source, globalScope);
}

/* 2-Technology.md#Constants */
double Tech::calLevelLocal(QList<std::pair<double, double>> &source) {
    double localScope =
        settings->value("rule/techlocalscope", 1.1).toDouble();
    return calLevel(source, localScope);
}

/* 2-Technology.md#Weight of ships */
double Tech::calWeightShip(int level) {
    double shipLevelPerWeight =
        settings->value("rule/shiplevelperweight", 10.0).toDouble();
    return level / shipLevelPerWeight;
}

/* 2-Technology.md#Weight of equipment, taking skill points into account */
double Tech::calWeightEquip(double requiredSP, double actualSP) {
    double skillPointWeightContrib =
        settings->value("rule/skillpointweightcontrib", 9.0).toDouble();
    return (skillPointWeightContrib * actualSP) / std::hypot(requiredSP, actualSP)
    + 1.0;
}

/* 2-Technology.md#How characteristic tech level are determined */
double Tech::techYearToCompact(int year) {
#pragma message(NOT_M_CONST)
    if(year <= 1924)
        return std::max(0.0, (year - 1908) / 16.0);
    else if(year <= 1933)
        return (year - 1921) / 3.0;
    else if(year <= 1941)
        return (year - 1933) / 2.0 + 4.0;
    else if(year <= 1946)
        return (year - 1941) + 8.0;
    else if(year <= 1948)
        return (year - 1946) / 2.0 + 13.0;
    else
        return (year - 1948) / 4.0 + 14.0;
}
