#include "tech.h"
#include <QDebug>

Tech::Tech()
{

}

double Tech::techIntToYear(int year) {
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

double Tech::calLevel(QMap<double, double> source,
                       const double scopeConstant) {
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
