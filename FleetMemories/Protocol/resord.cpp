/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

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

QString ResOrd::toString(bool simplified) const {
    if(!simplified) {
        //% "%1 Oil "
        QString oil = o != 0 ? (qtTrId("o-expr").arg(o)) : "";
        //% "%1 Ammo "
        QString explo = e != 0 ? (qtTrId("e-expr").arg(e)) : "";
        //% "%1 Steel "
        QString steel = s != 0 ? (qtTrId("s-expr").arg(s)) : "";
        //% "%1 Rubber "
        QString rubber = r != 0 ? (qtTrId("r-expr").arg(r)) : "";
        //% "%1 Aluminum "
        QString al = a != 0 ? (qtTrId("a-expr").arg(a)) : "";
        //% "%1 Tungsten "
        QString ww = w != 0 ? (qtTrId("w-expr").arg(w)) : "";
        //% "%1 Chromium "
        QString cr = c != 0 ? (qtTrId("c-expr").arg(c)) : "";
        return oil+explo+steel+rubber+al+ww+cr;
    }
    else {
        //% "%1O"
        QString oil = o != 0 ? (qtTrId("o-expr-s").arg(o)) : "";
        //% "%1E"
        QString explo = e != 0 ? (qtTrId("e-expr-s").arg(e)) : "";
        //% "%1S"
        QString steel = s != 0 ? (qtTrId("s-expr-s").arg(s)) : "";
        //% "%1R"
        QString rubber = r != 0 ? (qtTrId("r-expr-s").arg(r)) : "";
        //% "%1Al"
        QString al = a != 0 ? (qtTrId("a-expr-s").arg(a)) : "";
        //% "%1W"
        QString ww = w != 0 ? (qtTrId("w-expr-s").arg(w)) : "";
        //% "%1Cr"
        QString cr = c != 0 ? (qtTrId("c-expr-s").arg(c)) : "";
        return oil+explo+steel+rubber+al+ww+cr;
    }
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

bool ResOrd::addResourcesNonnegative(const ResOrd &amount,
                          const ResOrd &maximum) {
    o = std::max(std::min(o + amount.o, maximum.o), o);
    e = std::max(std::min(e + amount.e, maximum.e), e);
    s = std::max(std::min(s + amount.s, maximum.s), s);
    r = std::max(std::min(r + amount.r, maximum.r), r);
    a = std::max(std::min(a + amount.a, maximum.a), a);
    w = std::max(std::min(w + amount.w, maximum.w), w);
    c = std::max(std::min(c + amount.c, maximum.c), c);
    return true;
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
