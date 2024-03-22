#ifndef RESORD_H
#define RESORD_H

#include <QMap>
#include "kp.h"

#ifdef min
#undef min
#endif

typedef QMap<KP::ResourceType, int> ResTuple;

class ResOrd;

namespace User {
void setResources(int uid, ResOrd goal);
}

struct ResOrd
{
    ResOrd(ResTuple);
    ResOrd(int, int, int, int, int, int, int);

    QString toString() const;
    constexpr ResOrd& operator+=(const ResOrd& amount) {
        o += amount.o;
        e += amount.e;
        s += amount.s;
        r += amount.r;
        a += amount.a;
        w += amount.w;
        c += amount.c;
        return *this;
    }
    constexpr ResOrd& operator-=(const ResOrd& amount) {
        o -= amount.o;
        e -= amount.e;
        s -= amount.s;
        r -= amount.r;
        a -= amount.a;
        w -= amount.w;
        c -= amount.c;
        return *this;
    }
    const ResOrd operator*(qint64 amount) const {
        return ResOrd(
            o * amount,
            e * amount,
            s * amount,
            r * amount,
            a * amount,
            w * amount,
            c * amount
            );
    }
    const ResOrd operator*(double amount) const {
        return ResOrd(
            std::round(o * amount),
            std::round(e * amount),
            std::round(s * amount),
            std::round(r * amount),
            std::round(a * amount),
            std::round(w * amount),
            std::round(c * amount)
            );
    }
    constexpr ResOrd& operator*=(qint64 amount) {
        o *= amount;
        e *= amount;
        s *= amount;
        r *= amount;
        a *= amount;
        w *= amount;
        c *= amount;
        return *this;
    }
    bool addResources(const ResOrd &);
    bool addResources(const ResOrd &, const ResOrd &);
    bool spendResources(const ResOrd &);
    void cap(const ResOrd&);
    QByteArray resourceDesired() const;
    bool sufficient();

    int o; // oil
    int e; // explosives
    int s; // stell
    int r; // rubber
    int a; // aluminum
    int w; // tungsten
    int c; // chromium
};

#endif // RESORD_H
