#include "resord.h"

ResOrd::ResOrd(ResTuple input) {
    oil = explo = steel = rub = al = w = cr = 0;
    if(input.contains(KP::Oil))
        oil = input[KP::Oil];
    if(input.contains(KP::Explosives))
        explo = input[KP::Explosives];
    if(input.contains(KP::Steel))
        steel = input[KP::Steel];
    if(input.contains(KP::Rubber))
        rub = input[KP::Rubber];
    if(input.contains(KP::Aluminium))
        al = input[KP::Aluminium];
    if(input.contains(KP::Tungsten))
        w = input[KP::Tungsten];
    if(input.contains(KP::Chromium))
        cr = input[KP::Chromium];
}

ResOrd::ResOrd(int oil, int explo, int steel, int rub,
               int al, int w, int cr)
    : oil(oil), explo(explo), steel(steel), rub(rub),
      al(al), w(w), cr(cr) {

}

constexpr ResOrd& ResOrd::operator+=(const ResOrd& amount) {
    oil += amount.oil;
    explo += amount.explo;
    steel += amount.steel;
    rub += amount.rub;
    al += amount.al;
    w += amount.w;
    cr += amount.cr;
    return *this;
}

constexpr ResOrd& ResOrd::operator-=(const ResOrd& amount) {
    oil -= amount.oil;
    explo -= amount.explo;
    steel -= amount.steel;
    rub -= amount.rub;
    al -= amount.al;
    w -= amount.w;
    cr -= amount.cr;
    return *this;
}

bool ResOrd::addresources(const ResOrd& amount, const ResOrd &maximum) {
    operator+=(amount);
    if(!sufficient()){
        operator-=(amount);
        return false;
    }
    else {
        cap(maximum);
        return true;
    }
}

void ResOrd::cap(const ResOrd &cap) {
    using std::min;
    oil = min(oil, cap.oil);
    explo = min(explo, cap.explo);
    steel = min(steel, cap.steel);
    rub = min(rub, cap.rub);
    al = min(al, cap.al);
    w = min(al, cap.w);
    cr = min(al, cap.cr);
}

bool ResOrd::sufficient() {
    return !(oil < 0 || explo < 0 || steel < 0 || rub < 0
             || al < 0 || w < 0 || cr < 0);
}
