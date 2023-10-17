#ifndef TECH_H
#define TECH_H


#include "qmap.h"
class Tech
{
public:
    Tech();

    static double calLevel(QMap<double, double>, const double);
    static double techIntToYear(int);
};

#endif // TECH_H
