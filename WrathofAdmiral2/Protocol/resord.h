#ifndef RESORD_H
#define RESORD_H

#include <QMap>
#include "kp.h"

#ifdef min
#undef min
#endif

typedef QMap<KP::ResourceType, int> ResTuple;

class ResOrd
{
public:
    ResOrd(ResTuple);
    constexpr ResOrd& operator+=(const ResOrd&);
    constexpr ResOrd& operator-=(const ResOrd&);
    bool addresources(const ResOrd&, const ResOrd &);
    void cap(const ResOrd&);
    bool sufficient();

private:
    int oil;
    int explo;
    int steel;
    int rub;
    int al;
    int w;
    int cr;
};

#endif // RESORD_H
