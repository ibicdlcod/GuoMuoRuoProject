#ifndef TECH_H
#define TECH_H

#include "qmap.h"
#include <random>

class Tech
{
public:
    Tech();

    static double calCapable(double, double, double);
    static bool calExperiment(double, double, double, std::mt19937 &);
    static bool calExperiment2(double, double, double, double,
                               std::mt19937 &);
    static double calLevel(const QMap<double, double> &, const double);
    static double calLevelGlobal(const QMap<double, double> &);
    static double calLevelLocal(const QMap<double, double> &);
    static double calWeightEquip(double, double);
    static double calWeightShip(int);
    static double techYearToCompact(int);
};

#endif // TECH_H
