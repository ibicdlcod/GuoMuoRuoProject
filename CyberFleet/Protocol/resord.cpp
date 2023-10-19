#include "resord.h"
#include "kp.h"

ResOrd::ResOrd(ResTuple input) {
    oil = explo = steel = rub = al = w = cr = 0;
    if(input.contains(KP::O))
        oil = input[KP::O];
    if(input.contains(KP::E))
        explo = input[KP::E];
    if(input.contains(KP::S))
        steel = input[KP::S];
    if(input.contains(KP::R))
        rub = input[KP::R];
    if(input.contains(KP::A))
        al = input[KP::A];
    if(input.contains(KP::W))
        w = input[KP::W];
    if(input.contains(KP::C))
        cr = input[KP::C];
}

ResOrd::ResOrd(int oil, int explo, int steel, int rub,
               int al, int w, int cr)
    : oil(oil), explo(explo), steel(steel), rub(rub),
      al(al), w(w), cr(cr) {

}

QString ResOrd::toString() const {
    return qtTrId("Oil %1 Explo %2 Steel %3 Rub %4 Al %5 W %6 Cr %7")
        .arg(oil).arg(explo).arg(steel).arg(rub).arg(al).arg(w).arg(cr);
}

bool ResOrd::addResources(const ResOrd &amount) {
    operator+=(amount);
    if(!sufficient()){
        operator-=(amount);
        return false;
    }
    else {
        return true;
    }
}

bool ResOrd::addResources(const ResOrd &amount,
                          const ResOrd &maximum) {
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
    w = min(w, cap.w);
    cr = min(cr, cap.cr);
}

QByteArray ResOrd::resourceDesired() const {
    QJsonObject result;
    result["type"] = KP::DgramType::Message;
    result["msgtype"] = KP::MsgType::ResourceRequired;
    result["oil"] = oil;
    result["explo"] = explo;
    result["steel"] = steel;
    result["rub"] = rub;
    result["al"] = al;
    result["w"] = w;
    result["cr"] = cr;
    return QCborValue::fromJsonValue(result).toCbor();
}

bool ResOrd::sufficient() {
    return !(oil < 0 || explo < 0 || steel < 0 || rub < 0
             || al < 0 || w < 0 || cr < 0);
}
