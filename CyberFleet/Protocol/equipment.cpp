#include "equipment.h"
#include <QRegularExpression>
#include <QVariant>
#include <QMetaEnum>
#include <QSqlQuery>
#include "../Server/kerrors.h"

Equipment::Equipment(int equipId)
    : equipRegId(equipId){
    QStringList supportedLangs = {"ja_JP", "zh_CN", "en_US"};

    for(auto &lang: supportedLangs) {
        QSqlQuery query;
        query.prepare(
            "SELECT "+lang+" FROM EquipName "
                               "WHERE EquipID = :id;");
        query.bindValue(":id", equipId);
        if(!query.exec() || !query.isSelect()) {
            throw DBError(qtTrId("equip-local-name-lack"),
                          query.lastError());
            qCritical() << query.lastError();
        }
        else if(query.first()) {
            localNames[lang] = query.value(0).toString();
        }
    }

    QSqlQuery query;
    query.prepare(
        "SELECT Intvalue FROM EquipReg "
        "WHERE EquipID = :id AND Attribute = 'equiptype'");
    query.bindValue(":id", equipId);
    if(!query.exec() || !query.isSelect()) {
        throw DBError(qtTrId("equip-type-lack"),
                      query.lastError());
        qCritical() << query.lastError();
    }
    else if(query.first()) {
        type = EquipType(query.value(0).toInt());
    }
    QSqlQuery query2;
    query2.prepare(
        "SELECT Intvalue, Attribute FROM EquipReg "
        "WHERE EquipID = :id AND Attribute != 'equiptype'");
    query2.bindValue(":id", equipId);
    if(!query2.exec() || !query2.isSelect()) {
        throw DBError(qtTrId("equip-attr-lack"),
                      query2.lastError());
        qCritical() << query2.lastError();
    }
    else {
        while(query2.next()) {
            attr[query2.value(1).toString()]
                = query2.value(0).toInt();
        }
    }
}
Equipment::Equipment(const QJsonObject &input) {
    equipRegId = input["eid"].toInt();
    if(equipRegId == 0)
        return;
    QJsonObject lNames = input["name"].toObject();
    for(auto iter = lNames.constBegin(); iter != lNames.constEnd(); ++iter)
        localNames[iter->toString()] =
            lNames.value(iter->toString()).toString();
    type = EquipType(input["type"].toString());
    QJsonObject attrs = input["attr"].toObject();
    for(auto iter = attrs.constBegin(); iter != attrs.constEnd(); ++iter)
        attr[iter->toString()] =
            attrs.value(iter->toString()).toInt();
}/*
EquipType::EquipType(const QString &basis) {

    static QRegularExpression rehex(
                "^([a-zA-Z]+)(-[a-zA-Z]+(\\|[a-zA-Z]+)*)?(-\\d+)?$");
    auto m = rehex.match(basis);

    if(m.hasMatch()) {
        QString baseStr = m.captured(1);
        if(baseStr.isNull())
            base = Disabled;
        else {
            QMetaEnum info = QMetaEnum::fromType<BasicType>();
            base = BasicType(info.keyToValue(
                                 baseStr.toLatin1().constData()));
        }
        QString flagsStr = m.captured(2);
        if(flagsStr.isNull())
            flags = TypeFlags();
        else {
            flagsStr.remove(0, 1);
            QMetaEnum info = QMetaEnum::fromType<TypeFlags>();
            auto flagsnum = info.keysToValue(flagsStr.toLatin1().constData());
            if(flagsnum == -1)
                flags = TypeFlags();
            else
                flags = TypeFlags(flagsnum);
        }
        QString sizeStr = m.captured(4);
        if(sizeStr.isNull())
            size = 0;
        else {
            sizeStr.remove(0, 1);
            size = sizeStr.toInt();
        }
    }
    else {
        base = Disabled;
        flags = TypeFlags();
        size = 0;
    }
}

QString EquipType::toString() const {
    QString baseStr = QVariant::fromValue(base).toString();
    QMetaEnum info = QMetaEnum::fromType<TypeFlags>();
    if(!flags.testFlags(TypeFlags())) {
        baseStr.append("-");
        baseStr.append(info.valueToKeys(flags));
    }
    if(size > 0) {
        baseStr.append("-");
        baseStr.append(QString::number(size));
    }
    return baseStr;
}

const ResOrd EquipType::devResBase() const {
    using namespace KP;
    ResTuple basic;
    bool flak = flags.testFlag(Flak);
    bool torp = flags.testFlag(Torp);
    bool dive = flags.testFlag(Dive);
    bool lb = flags.testFlag(LB);
    bool jet = flags.testFlag(Jet);
    bool recon = flags.testFlag(Recon);
    bool cannon = flags.testFlag(Cannon);
    bool convoy = flags.testFlag(Convoy);
    switch(base) {
    case Disabled: break;
    case MainGun:
        basic[Explosives] = size * 10;
        basic[Steel] = size * 5;
        basic[Aluminium] = flak ? 1 : 0;
        break;
    case SupportGun:
        basic[Explosives] = size * 5;
        basic[Steel] = size * 5;
        basic[Aluminium] = flak ? 1 : 0;
        break;
    case Torpedo:
        basic[Oil] = size * 10;
        basic[Explosives] = size * 10;
        basic[Steel] = size * 5;
        break;
    case Midget:
        basic[Oil] = 20;
        basic[Explosives] = 10;
        basic[Steel] = 5;
        basic[Chromium] = 2;
        break;
    case Plane:
        basic[Oil] = 10 + ((torp || dive) ? 5 : 0)
                + (lb ? 3 : 0) + (jet ? 10 : 0);
        basic[Explosives] = 2 + ((torp || dive) ? 5 : 0)
                + (lb ? 5 : 0) + (recon ? -1 : 0);
        basic[Aluminium] = 20 + (lb ? 2 : 0) + (jet ? 10 : 0);
        basic[Rubber] = 2;
        basic[Chromium] = jet ? 5 : 0;
        break;
    case Seaplane:
        basic[Oil] = 5 + ((torp || dive) ? 2 : 0);
        basic[Explosives] = (torp || dive) ? 2 : 0;
        basic[Aluminium] = 10;
        break;
    case Flyingboat:
        basic[Oil] = 50;
        basic[Explosives] = recon ? 0 : 20;
        basic[Aluminium] = 10;
        break;
    case Autogyro:
        basic[Aluminium] = 10;
        break;
    case Liason:
        basic[Aluminium] = 5;
        break;
    case DepthC:
        basic[Explosives] = 10;
        basic[Steel] = 5;
        break;
    case Sonar:
        basic[Steel] = 5;
        basic[Aluminium] = 10;
        break;
    case AA:
        basic[Explosives] = 10 + (cannon ? 5 : 0);
        basic[Steel] = 5 + (cannon ? 3 : 0);
        basic[Aluminium] = 2;
        break;
    case AADirector:
        basic[Explosives] = 5;
        basic[Aluminium] = 2;
        break;
    case APShell:
        basic[Explosives] = 20;
        basic[Steel] = 40;
        basic[Tungsten] = 25;
        break;
    case ALShell:
        basic[Explosives] = 30;
        basic[Steel] = 20;
        break;
    case ALRocket:
        basic[Explosives] = 20;
        basic[Steel] = 10;
        basic[Chromium] = 5;
        break;
    case ALCraft:
        basic[Oil] = 5;
        basic[Steel] = 20;
        basic[Rubber] = 30;
        basic[Tungsten] = 30;
        break;
    case ALTank:
        basic[Oil] = 5;
        basic[Steel] = 30;
        basic[Rubber] = 40;
        basic[Tungsten] = 30;
        break;
    case Drum:
        basic[Steel] = 5;
        break;
    case TPCraft:
        basic[Oil] = convoy ? 20 : 5;
        basic[Steel] = convoy ? 20 : 10;
        break;
    case Radar:
        basic[Steel] = size * 20;
        basic[Aluminium] = size * 25 + (flak ? 1 : 0);
        break;
    case RadarSub:
        basic[Steel] = 15;
        basic[Aluminium] = 20;
        break;
    case Engine:
        basic[Oil] = 10;
        basic[Steel] = 15;
        basic[Chromium] = 30;
        break;
    case Bulge:
        basic[Steel] = size * 15;
        break;
    case SearchLight:
        basic[Tungsten] = 50;
        break;
    case StarShell:
        basic[Explosives] = 20;
        basic[Steel] = 10;
        basic[Aluminium] = 10;
        break;
    case Repair:
        basic[Oil] = 10;
        basic[Explosives] = 5;
        basic[Steel] = 20;
        basic[Aluminium] = 10;
        break;
    case Replenish:
        basic[Oil] = 30;
        basic[Explosives] = 30;
        break;
    case Food: break; // can't develop
    case CommandFac: break; // can't develop
    case AircraftPs: break; // can't develop
    case SurfacePs: break; // can't develop
    case TorpBoat:
        basic[Oil] = 5;
        basic[Explosives] = 5;
        basic[Steel] = 5;
        break;
    }
    return ResOrd(basic);
}

EquipDef::EquipDef(int id,
                   QString &&name,
                   QString &&type,
                   QMap<AttrType, int> &&attr,
                   QStringList &&customflags)
    : id(id),
      name(name),
      type(type),
      attr(attr),
      customflags(customflags) {
}

bool EquipDef::canDevelop([[maybe_unused]] CSteamID userid) const {
    return attr[Developenabled] == 1;
}

const ResOrd EquipDef::devRes() const {
    return type.devResBase() * (qint64)attr[Rarity];
}

int EquipDef::getRarity() const {
    return attr[Rarity];
}
*/
