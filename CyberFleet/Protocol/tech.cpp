#include "tech.h"
#include <QDebug>

Tech::Tech()
{

}

double Tech::calCapable(double globalTech, double localTech,
                        double wantedTech) {
    static const double maxDeterimental = 3.0;
    double min = std::min(globalTech, localTech);
    double max = std::max(globalTech, localTech);
    double detrimentalEffect = 0.0;
    if(min < wantedTech) {
        double lag = wantedTech - min;
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

double Tech::calLevel(const QMap<double, double> &source,
                      const double scopeConstant) {
    /* Note QMap always orders by key, as opposed to QHash */
    if(scopeConstant <= 1.0) {
        qCritical() << qtTrId("scope-constant-less-than-1");
        return 0.0;
    }
    double result = 0.0;
    double weightSum = 0.0;
    QList<double> components = source.keys();
    for(auto iter = components.rbegin();
         iter != components.rend();
         iter++) {
        if(*iter < 0.0) {
            qCritical() << qtTrId("tech-level-less-than-0");
            return 0.0;
        }
        double weight = source[*iter];
        if(weight < 0.0) {
            qCritical() << qtTrId("tech-weight-less-than-0");
            return 0.0;
        }
        weightSum += weight;
        result += (*iter) * (pow(scopeConstant, weight) - 1.0)
                  * (pow(scopeConstant, -weightSum));
    }
    return result;
}

double Tech::calLevelGlobal(const QMap<double, double> &source) {
    static const double globalScope = 1.02;
    return calLevel(source, globalScope);
}

double Tech::calLevelLocal(const QMap<double, double> &source) {
    static const double localScope = 1.1;
    return calLevel(source, localScope);
}

double Tech::calWeightShip(int level) {
    static const double shipLevelPerWeight = 10.0;
    return level / shipLevelPerWeight;
}

double calWeightEquip(double requiredSP, double actualSP) {
    return (9 * actualSP)
               / pow((requiredSP * requiredSP + actualSP * actualSP), 0.5)
           + 1;
}

double Tech::techYearToCompact(int year) {
    if(year <= 1921)
        return 0;
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
