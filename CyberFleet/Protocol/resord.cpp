#include "resord.h"
#include <QSettings>
#include "kp.h"

extern std::unique_ptr<QSettings> settings;

ResOrd::ResOrd(ResTuple input) {
    o = e = s = r = a = w = c = 0;
    if(input.contains(KP::O))
        o = input[KP::O];
    if(input.contains(KP::E))
        e = input[KP::E];
    if(input.contains(KP::S))
        s = input[KP::S];
    if(input.contains(KP::R))
        r = input[KP::R];
    if(input.contains(KP::A))
        a = input[KP::A];
    if(input.contains(KP::W))
        w = input[KP::W];
    if(input.contains(KP::C))
        c = input[KP::C];
}

ResOrd::ResOrd(int oil, int explo, int steel, int rub,
               int al, int w, int cr)
    : o(oil), e(explo), s(steel), r(rub),
      a(al), w(w), c(cr) {

}

QString ResOrd::toString() const {
    //% "%1 Oil"
    QString oil = o != 0 ? (qtTrId("%1 Oil").arg(o)) : "";
    //% "%1 Ammo "
    QString explo = e != 0 ? (qtTrId("%1 Ammo").arg(e)) : "";
    //% "%1 Steel "
    QString steel = s != 0 ? (qtTrId("%1 Steel").arg(s)) : "";
    //% "%1 Rubber "
    QString rubber = r != 0 ? (qtTrId("%1 Rubber").arg(r)) : "";
    //% "%1 Aluminum "
    QString al = a != 0 ? (qtTrId("%1 Al").arg(a)) : "";
    //% "%1 Tungsten "
    QString ww = w != 0 ? (qtTrId("%1 W").arg(w)) : "";
    //% "%1 Chromium "
    QString cr = c != 0 ? (qtTrId("%1 Cr").arg(c)) : "";

    return oil+explo+steel+rubber+al+ww+cr;
}

bool ResOrd::addResources(const ResOrd &amount) {
    int maxRes = settings->value("rule/maxresources", 3600000).toInt();
    operator+=(amount);
    cap(ResOrd(maxRes,
               maxRes,
               maxRes,
               maxRes,
               maxRes,
               maxRes,
               maxRes));
    return true;
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

/* attempt to spend resources, will not change if failed */
bool ResOrd::spendResources(const ResOrd &amount) {
    operator-=(amount);
    if(!sufficient()){
        operator+=(amount);
        return false;
    }
    else {
        return true;
    }
}

/* 3-Resources.md#Stockpile cap */
void ResOrd::cap(const ResOrd &cap) {
    using std::min;
    o = min(o, cap.o);
    e = min(e, cap.e);
    s = min(s, cap.s);
    r = min(r, cap.r);
    a = min(a, cap.a);
    w = min(w, cap.w);
    c = min(c, cap.c);
}

/* convert to Server message */
QByteArray ResOrd::resourceDesired() const {
    QJsonObject result;
    result["type"] = KP::DgramType::Message;
    result["msgtype"] = KP::MsgType::ResourceRequired;
    result["oil"] = o;
    result["explo"] = e;
    result["steel"] = s;
    result["rub"] = r;
    result["al"] = a;
    result["w"] = w;
    result["cr"] = c;
    return QCborValue::fromJsonValue(result).toCbor();
}

bool ResOrd::sufficient() {
    return !(o < 0 || e < 0 || s < 0 || r < 0
             || a < 0 || w < 0 || c < 0);
}
