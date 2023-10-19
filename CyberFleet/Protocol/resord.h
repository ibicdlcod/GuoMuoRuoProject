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

class ResOrd
{
public:
    ResOrd(ResTuple);
    ResOrd(int, int, int, int, int, int, int);

    QString toString() const;
    constexpr ResOrd& operator+=(const ResOrd& amount) {
        oil += amount.oil;
        explo += amount.explo;
        steel += amount.steel;
        rub += amount.rub;
        al += amount.al;
        w += amount.w;
        cr += amount.cr;
        return *this;
    }
    constexpr ResOrd& operator-=(const ResOrd& amount) {
        oil -= amount.oil;
        explo -= amount.explo;
        steel -= amount.steel;
        rub -= amount.rub;
        al -= amount.al;
        w -= amount.w;
        cr -= amount.cr;
        return *this;
    }
    const ResOrd operator*(qint64 amount) const {
        return ResOrd(
                    oil * amount,
                    explo * amount,
                    steel * amount,
                    rub * amount,
                    al * amount,
                    w * amount,
                    cr * amount
                    );
    }
    constexpr ResOrd& operator*=(qint64 amount) {
        oil *= amount;
        explo *= amount;
        steel *= amount;
        rub *= amount;
        al *= amount;
        w *= amount;
        cr *= amount;
        return *this;
    }
    bool addResources(const ResOrd&);
    bool addResources(const ResOrd&, const ResOrd &);
    void cap(const ResOrd&);
    QByteArray resourceDesired() const;
    bool sufficient();

    friend void User::setResources(int uid, ResOrd goal);

//private:
    int oil;
    int explo;
    int steel;
    int rub;
    int al;
    int w;
    int cr;
};

#endif // RESORD_H
