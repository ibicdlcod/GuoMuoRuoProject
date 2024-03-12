#ifndef TECH_H
#define TECH_H

#include <QList>
#include <random>

class Tech
{
public:
    Tech();

    static double calCapable(double, double, double);
    static bool calExperiment(double, double, double, std::mt19937 &);
    static bool calExperiment2(double, double, double, double,
                               std::mt19937 &);
    static double calLevel(QList<std::pair<double, double>> &, const double);
    static double calLevelGlobal(QList<std::pair<double, double>> &);
    static double calLevelLocal(QList<std::pair<double, double>> &);
    static double calWeightEquip(double, double);
    static double calWeightShip(int);
    static double techYearToCompact(int);
};

#endif // TECH_H
