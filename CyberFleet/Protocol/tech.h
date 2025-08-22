#ifndef TECH_H
#define TECH_H

#include <QList>
#include <random>

namespace Tech
{
    double calCapable(double, double, double);
    bool calExperiment(double, double, double, std::mt19937 &);
    bool calExperiment2(double, double, double, double,
                        std::mt19937 &);
    double calExperimentRate(double, double, double, double);
    double calLevel(QList<std::pair<double, double>> &, const double);
    double calLevelGlobal(QList<std::pair<double, double>> &);
    double calLevelLocal(QList<std::pair<double, double>> &);
    double calWeightEquip(double, double);
    double calWeightShip(int);
    double techYearToCompact(int);
};

#endif // TECH_H
