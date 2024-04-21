#include "tech.h"
#include <QDebug>
#include <QSettings>

extern std::unique_ptr<QSettings> settings;

double Tech::calCapable(double globalTech, double localTech,
                        double wantedTech) {
    double maxDeterimental =
        settings->value("rule/techcombinedeffects", 3.0).toDouble();
    double min = std::min(globalTech, localTech);
    double max = std::max(globalTech, localTech);
    double detrimentalEffect = 0.0;
    if(min < wantedTech) {
        double lag = wantedTech - min;
        /* 0.5 is not a magic const because how this function behaves */
        detrimentalEffect = maxDeterimental * lag / pow((1 + lag * lag), 0.5);
    }
    return std::max(min, max - detrimentalEffect);
}

bool Tech::calExperiment(const double wantedTech,
                         const double currentTech,
                         const double sigmaConstant,
                         std::mt19937 &generator) {
    assert(sigmaConstant > 0.0);
    std::normal_distribution d{wantedTech, sigmaConstant};
    double random_double = d(generator);
    return random_double <= currentTech;
}

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

double Tech::calLevel(QList<std::pair<double, double>> &source,
                      const double scopeConstant) {
    /* sort by key desc */
    std::sort(source.begin(), source.end(),
              [](std::pair<double, double> a, std::pair<double, double> b){
        return a.first > b.first;
    });
    if(scopeConstant <= 1.0) {
        qCritical() << qtTrId("scope-constant-less-than-1");
        return 0.0;
    }
    double result = 0.0;
    double weightSum = 0.0;
    for(auto iter = source.begin();
         iter != source.end();
         iter++) {
        if(iter->first < 0.0) {
            qCritical() << qtTrId("tech-level-less-than-0");
            return 0.0;
        }
        double weight = iter->second;
        if(weight < 0.0) {
            //qCritical() << qtTrId("tech-weight-less-than-0");
            weight = 0.0;
        }
        weightSum += weight;
        result += (iter->first) * (pow(scopeConstant, weight) - 1.0)
                  * (pow(scopeConstant, -weightSum));
    }
    return result;
}

double Tech::calLevelGlobal(QList<std::pair<double, double>> &source) {
    double globalScope =
        settings->value("rule/techglobalscope", 1.02).toDouble();
    return calLevel(source, globalScope);
}

double Tech::calLevelLocal(QList<std::pair<double, double>> &source) {
    double localScope =
        settings->value("rule/techlocalscope", 1.1).toDouble();
    return calLevel(source, localScope);
}

double Tech::calWeightShip(int level) {
    double shipLevelPerWeight =
        settings->value("rule/shiplevelperweight", 10.0).toDouble();
    return level / shipLevelPerWeight;
}

double calWeightEquip(double requiredSP, double actualSP) {
    double skillPointWeightContrib =
        settings->value("rule/skillpointweightcontrib", 9.0).toDouble();
    /* 0.5 is not a magic const because how this function behaves */
    return (skillPointWeightContrib * actualSP)
               / pow((requiredSP * requiredSP + actualSP * actualSP), 0.5)
           + 1.0;
}

double Tech::techYearToCompact(int year) {
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
