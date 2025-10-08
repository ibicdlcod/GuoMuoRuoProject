#include "equipment.h"
#include <QRegularExpression>
#include <QVariant>
#include <QMetaEnum>
#include <QSqlQuery>
#include <QSettings>
#include "../Server/kerrors.h"
#include "tech.h"
#include "utility.h"

extern std::unique_ptr<QSettings> settings;

Equipment::Equipment(int equipId)
    : equipRegId(equipId){
    if(equipId == 0) {
        return;
    }
    QStringList supportedLangs = {"ja_JP", "zh_CN", "en_US"};

    for(auto &lang: supportedLangs) {
        QSqlQuery query;
        query.prepare(
            "SELECT "+lang+" FROM EquipName "
                               "WHERE EquipID = :id;");
        query.bindValue(":id", equipId);
        if(!query.exec() || !query.isSelect()) {
            //% "Local language (%1) for equipment name not found!"
            throw DBError(qtTrId("equip-local-name-lack").arg(lang),
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
        //% "Fetch equipment type failure!"
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
        //% "Fetch equipment attributes failure!"
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
    for(auto &lang: lNames.keys()) {
        localNames[lang] =
            lNames.value(lang).toString();
    }
    type = EquipType(input["type"].toString());
    QJsonObject attrs = input["attr"].toObject();
    for(auto &attrI: attrs.keys()) {
        attr[attrI] =
            attrs.value(attrI).toInt();
    }
}

int Equipment::operator<=>(const Equipment &other) const {
    int typeResult = this->type.getTypeSort() - other.type.getTypeSort();
    if(typeResult == 0)
        return equipRegId - other.equipRegId;
    else
        return typeResult;
}

/* not operator!= because QObject don't have == */
bool Equipment::isNotEqual(const Equipment &other) const {
    return operator<=>(other) != 0;
}

QString Equipment::toString(QString lang) const {
    return localNames[lang].isEmpty() ? localNames["ja_JP"] : localNames[lang];
}

const QString Equipment::attrStr() const {
    QString result;
    for(auto iter = attr.constKeyValueBegin();
         iter != attr.constKeyValueEnd();
         ++iter) {
        auto attrName = iter->first;
        auto attrValue = iter->second;
        if(attrName.localeAwareCompare("Tech") == 0
            || attrName.contains("Father")
            || attrName.contains("Mother")
            || attrName.localeAwareCompare("Disallowmassproduction") == 0) {
            continue;
        }
        else if(attrValue != 0) {
            QString toTranslate = QString("equip-attr-")
                                  + attrName.toLower();
            result = result + qtTrId(toTranslate.toUtf8())
                     + " "
                     + (attrValue > 0 ? "+" : "")
                     + QString::number(attrValue) + " ";
        }
    }
    if(result.endsWith("")) {
        result.erase(result.constEnd() - 1, result.constEnd());
    }
    return result;
}

const QString Equipment::attrPrimaryStr() const {
    QString result;
    auto attrName = type.getPrimaryAttr();
    if(attrName.localeAwareCompare("Tech") == 0
        || attrName.contains("Father")
        || attrName.contains("Mother")
        || attrName.localeAwareCompare("Disallowmassproduction") == 0) {
        return "";
    }
    auto attrValue = attr[attrName];
    QString toTranslate = QString("equip-attr-")
                          + attrName.toLower();
    result = result + qtTrId(toTranslate.toUtf8())
             + " "
             + (attrValue > 0 ? "+" : "")
             + QString::number(attrValue);

    return result;
}

/* 4.3-Development.md#Resource cost */
const ResOrd Equipment::devRes() const {
    qint64 devResScale = settings->value("rule/devresscale", 10).toLongLong();
    return type.devResBase() * (qint64)std::round((getTech() + 1.0)
                                                   * devResScale);
}

/* 4.3-Development.md#Development time */
const int Equipment::devTimeInSec() const {
    qint64 devTimebase = settings->value("rule/devtimebase", 6).toLongLong();
    qint64 devResScale = settings->value("rule/devresscale", 10).toLongLong();
    return devTimebase * (qint64)std::round((getTech() + 1.0) * devResScale);
}

/* under new doctrine this should always return true */
bool Equipment::disallowMassProduction() const {
    return attr.contains("Disallowmassproduction")
           && attr.value("Disallowmassproduction") > 0;
}

bool Equipment::disallowProduction() const {
    return type.isVirtual() || (
               attr.contains("Disallowmassproduction")
               && attr.value("Disallowmassproduction") == -1);
}

int Equipment::getId() const {
    return equipRegId;
}

double Equipment::getTech() const {
    return Tech::techYearToCompact(attr["Tech"]);
}

bool Equipment::isInvalid() const {
    return equipRegId == 0;
};

/* 4.5-Skillpoints.md#Standard skill points */
int Equipment::skillPointsStd() const {
    double skillPointFactor = settings->value("rule/skillpointfactor",
                                              1.25).toDouble();
    double skillPointBase = settings->value("rule/skillpointbase",
                                            10000.0).toDouble();
    return std::lround(std::pow(skillPointFactor, getTech())
                       * skillPointBase);
}

bool Equipment::canEquip(Ship *ship) const
{
    if(type.isLb()) {
        return Utility::checkMask(ship->getId(), 0x000f0000, 0x000C0000);
    }
    switch(type.getSpecial())
    {
    case KP::NonSpecial: [[fallthrough]];
    case KP::LimitedNightPlane:
        if(type.isRadar()) {
            switch(type.getSize()) {
            case 7:
                return Utility::checkMask(ship->getId(), 0x000f0000, 0x00070000);
            case 1:
                if(Utility::checkMask(ship->getId(), 0x00ff0fff, 0x001A0301))
                    return false;
                return !Utility::checkMask(ship->getId(), 0x000f0000, 0x00070000);
            case 2:
            {
                if(ship->customFlags.contains("bigradar")) {
                    if(ship->customFlags["bigradar"] == 1)
                        return true;
                    if(ship->customFlags["bigradar"] == -1)
                        return false;
                }
                if(Utility::checkMask(ship->getId(), 0x00ff0000, 0x00220000))
                    return getId() == 124; // Z-class
                if(Utility::checkMask(ship->getId(), 0x00ff0f00, 0x00120B00))
                    return true; // 秋月型
                if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00030000))
                    return true;
                if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00040000))
                    return true;
                if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00050000))
                    return true;
                if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00060000))
                    return true;
                if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00080000))
                    return true;
                return false;
            }
            case 3:
                return Utility::checkMask(ship->getId(), 0x000f0000, 0x00050000);
            }
        }
        else if(type.isPatrol()) {
            if(type.getSize() == 2) {
                if(ship->customFlags.contains("patrolautogyro")) {
                    if(ship->customFlags["patrolautogyro"] == 1)
                        return true;
                    if(ship->customFlags["patrolautogyro"] == -1)
                        return false;
                }
                if(Utility::checkMask(ship->getId(), 0x000f4000, 0x00054000))
                    return true;
                if(Utility::checkMask(ship->getId(), 0x000f4000, 0x00044000))
                    return true;
                if(Utility::checkMask(ship->getId(), 0x000f4000, 0x00034000))
                    return true;
                if(Utility::checkMask(ship->getId(), 0x000f0000, 0x000B0000))
                    return true;
                if(Utility::checkMask(ship->getId(), 0xffff8000, 0x30158000))
                    return true; // 大和改二、武蔵改二
            }
            if(ship->customFlags.contains("patrolliason")) {
                if(ship->customFlags["patrolliason"] == 1)
                    return true;
                if(ship->customFlags["patrolliason"] == -1)
                    return false;
            }
            if(Utility::checkMask(ship->getId(), 0x000f2000, 0x00062000))
                return true; // 加賀改二護、Victorious/改(or anyone similar)
            if(Utility::checkMask(ship->getId(), 0x000f1000, 0x00061000))
                return true;
            if(Utility::checkMask(ship->getId(), 0xffffff00, 0x30154200))
                return true; // 伊勢型改二
            return false;
        }
        else if(type.isCarrierPlane()) {
            if(Utility::checkMask(ship->getId(), 0x000f0000, 0x000C0000)) {
                return true;
            }
            bool result = true;
            bool canEquipRecon = Utility::checkMask(ship->getId(), 0x000f0000, 0x00060000);
            if(Utility::checkMask(ship->getId(), 0xffffff00, 0x30154200))
                canEquipRecon = true; // 伊勢型改二
            if(Utility::checkMask(ship->getId(), 0x00ffff00, 0x00163700))
                canEquipRecon = false; // 大鷹型
            if(Utility::checkMask(ship->getId(), 0xffffff00, 0x30163700))
                canEquipRecon = true; // 大鷹型改二
            bool canEquipDiveBomb = Utility::checkMask(ship->getId(), 0x000f0000, 0x00060000);
            if(ship->customFlags.contains("divebomber")) {
                if(ship->customFlags["divebomber"] == 1)
                    canEquipDiveBomb = true;
                if(ship->customFlags["divebomber"] == -1)
                    canEquipDiveBomb = false;
            }
            bool canEquipTorpBomb = Utility::checkMask(ship->getId(), 0x000f0000, 0x00060000);
            if(ship->customFlags.contains("torpbomber")) {
                if(ship->customFlags["torpbomber"] == 1)
                    canEquipTorpBomb = true;
                if(ship->customFlags["torpbomber"] == -1)
                    canEquipTorpBomb = false;
            }
            bool canEquipFighter = Utility::checkMask(ship->getId(), 0x000f0000, 0x00060000);
            if(ship->customFlags.contains("fighter")) {
                if(ship->customFlags["fighter"] == 1)
                    canEquipFighter = true;
                if(ship->customFlags["fighter"] == -1)
                    canEquipFighter = false;
            }

            result = result && (!type.isRecon() || canEquipRecon);
            result = result && (!type.isDiveBomber() || canEquipDiveBomb);
            result = result && (!type.isTorpBomber() || canEquipTorpBomb);
            result = result && (!type.isFighter() || canEquipFighter);

            return result;
        }
        else if(type.isSeaplane()) {
            if(Utility::checkMask(ship->getId(), 0x000f0000, 0x000C0000)) {
                return true;
            }
            if(type.getSize() == 1 && Utility::checkMask(ship->getId(), 0x000ff000, 0x00074000)) {
                return true;
            }
            bool result = true;
            bool canEquipRecon =
                Utility::checkMask(ship->getId(), 0x000f0000, 0x00030000)
                || Utility::checkMask(ship->getId(), 0x000f0000, 0x00040000)
                || Utility::checkMask(ship->getId(), 0x000f0000, 0x00050000)
                || Utility::checkMask(ship->getId(), 0x000f0000, 0x00080000);
            if(ship->customFlags.contains("sprecon")) {
                if(ship->customFlags["sprecon"] == 1)
                    canEquipRecon = true;
                if(ship->customFlags["sprecon"] == -1)
                    canEquipRecon = false;
            }
            bool canEquipDiveBomb =
                Utility::checkMask(ship->getId(), 0x000f4000, 0x00034000)
                || Utility::checkMask(ship->getId(), 0x000f4000, 0x00044000)
                || Utility::checkMask(ship->getId(), 0x000f4000, 0x00054000)
                || Utility::checkMask(ship->getId(), 0x000f0000, 0x00080000)
                || Utility::checkMask(ship->getId(), 0x000f0000, 0x00090000);
            if(ship->customFlags.contains("spbomber")) {
                if(ship->customFlags["spbomber"] == 1)
                    canEquipDiveBomb = true;
                if(ship->customFlags["spbomber"] == -1)
                    canEquipDiveBomb = false;
                if(ship->customFlags["spbomber"] == 194)
                    canEquipDiveBomb = getId() == 194; // Laté 298B
            }
            bool canEquipFighter =
                Utility::checkMask(ship->getId(), 0x000f4000, 0x00034000)
                || Utility::checkMask(ship->getId(), 0x000f4000, 0x00044000)
                || Utility::checkMask(ship->getId(), 0x000f4000, 0x00054000)
                || Utility::checkMask(ship->getId(), 0x000f0000, 0x00080000);
            if(ship->customFlags.contains("spfighter")) {
                if(ship->customFlags["spfighter"] == 1)
                    canEquipFighter = true;
                if(ship->customFlags["spfighter"] == -1)
                    canEquipFighter = false;
            }

            result = result && (!type.isRecon() || canEquipRecon);
            result = result && (!type.isDiveBomber() || canEquipDiveBomb);
            result = result && (!type.isFighter() || canEquipFighter);

            return result;
        }
        else if(type.isTorp()) {
            if(!type.isSurface()) {
                return Utility::checkMask(ship->getId(), 0x000f0000, 0x00070000);
            }
            else {
                if(ship->customFlags.contains("torp")) {
                    if(ship->customFlags["torp"] == 1)
                        return true;
                    if(ship->customFlags["torp"] == -1)
                        return false;
                }
                if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00020000))
                    return true;
                if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00030000))
                    return true;
                if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00040000))
                    return true;
                if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00070000))
                    return true;
                if(Utility::checkMask(ship->getId(), 0x000f1000, 0x00051000))
                    return true;
                if(Utility::checkMask(ship->getId(), 0x000ff000, 0x00011000))
                    return true;
                return false;
            }
        }
        else if(type.isSecGun()) {
            if(type.isFlak() && Utility::checkMask(ship->getId(), 0x000f0000, 0x000C0000)) {
                return true;
            }
            if(type.getSize() == 3) {
                if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00040000))
                    return true;
                if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00050000))
                    return true;
                if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00060000))
                    return true;
                return false;
            }
            else {
                if(ship->customFlags.contains("secgun")) {
                    if(ship->customFlags["secgun"] == 1)
                        return true;
                    if(ship->customFlags["secgun"] == -1)
                        return false;
                    if(ship->customFlags["secgun"] == 12)
                        return getId() == 524; //12cm単装高角砲＋25mm機銃増備
                }
                if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00030000))
                    return true;
                if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00040000))
                    return true;
                if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00050000))
                    return true;
                if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00060000))
                    return true;
                if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00080000))
                    return true;
                return false;
            }
        }
        else if(type.isMainGun()) {
            if(type.isFlak() && Utility::checkMask(ship->getId(), 0x000f0000, 0x000C0000)) {
                return true;
            }
            switch(type.getSize()) {
            case 1:
                if(ship->customFlags.contains("smallgun")) {
                    if(ship->customFlags["smallgun"] == 1)
                        return true;
                    if(ship->customFlags["smallgun"] == -1)
                        return false;
                    if(ship->customFlags["smallgun"] == 12)
                        return getId() == 48; //12cm単装高角砲
                    if(ship->customFlags["smallgun"] == 382)
                        return getId() == 229 || getId() == 379 || getId() == 382;
                }
                if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00010000))
                    return true;
                if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00020000))
                    return true;
                if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00030000))
                    return true;
                if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00080000))
                    return true;
                return false;
            case 2:
                if(ship->customFlags.contains("midgun")) {
                    if(ship->customFlags["midgun"] == 1)
                        return true;
                    if(ship->customFlags["midgun"] == -1)
                        return false;
                }
                if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00030000))
                    return true;
                if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00040000))
                    return true;
                return false;
            case 3:
                if(ship->customFlags.contains("midgun")) {
                    if(ship->customFlags["midgun"] == 1)
                        return true;
                    if(ship->customFlags["midgun"] == -1)
                        return false;
                }
                if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00040000))
                    return true;
                if(Utility::checkMask(ship->getId(), 0x000f1000, 0x00050000))
                    return true;
                return false;
            case 4:
                if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00050000))
                    return true;
                return false;
            case 5:
                if(Utility::checkMask(ship->getId(), 0x000f1000, 0x00050000))
                    return true;
                return false;
            case 6:
                if(Utility::checkMask(ship->getId(), 0x000f8000, 0x00058000))
                    return true;
                if(Utility::checkMask(ship->getId(), 0xffff0f00, 0x30150300))
                    return true; // 長門改二、陸奥改二
                return false;
            default: return false;
            }

        }
        return false;
    case KP::MidgetSub:
        if(ship->customFlags.contains("torp")) {
            if(ship->customFlags["smallgun"] == -2)
                return false;
        }
        if(Utility::checkMask(ship->getId(), 0x000f2000, 0x00082000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000f2000, 0x00032000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000f2000, 0x00042000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00070000))
            return true;
        return false;
    case KP::DepthCharge:
        if(ship->customFlags.contains("depthcharge")) {
            if(ship->customFlags["depthcharge"] == 1)
                return true;
            if(ship->customFlags["depthcharge"] == -1)
                return false;
        }
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00020000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00030000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00010000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00080000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x000A0000))
            return true;
        return false;
    case KP::Ballon: [[fallthrough]];
    case KP::Smoke:
        if(ship->customFlags.contains("smoke")) {
            if(ship->customFlags["smoke"] == 1)
                return true;
            if(ship->customFlags["smoke"] == -1)
                return false;
        }
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00020000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00030000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00010000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00040000))
            return true;
        return false;
    case KP::Sonar:
        if(type.getSize() == 3) {
            if(ship->customFlags.contains("bigsonar")) {
                if(ship->customFlags["bigsonar"] == 1)
                    return true;
                if(ship->customFlags["bigsonar"] == -1)
                    return false;
            }
            if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00050000))
                return true;
            if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00040000))
                return true;
            if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00060000))
                return true;
            if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00080000))
                return true;
            if(Utility::checkMask(ship->getId(), 0x000f0000, 0x000A0000))
                return true;
            if(Utility::checkMask(ship->getId(), 0x000f5000, 0x00035000))
                return true; // 潜水母舰
            return false;
        }
        if(ship->customFlags.contains("sonar")) {
            if(ship->customFlags["sonar"] == 1)
                return true;
            if(ship->customFlags["sonar"] == -1)
                return false;
        }
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00020000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00030000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00010000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00070000))
            return true;
        return false;
    case KP::APShell:
        return Utility::checkMask(ship->getId(), 0x000f0000, 0x00050000);
    case KP::AntilandShell:
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00040000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00050000))
            return true;
        if(Utility::checkMask(ship->getId(), 0xffffffff, 0x3F182602))
            return true; // 三隈改二特
        return false;
    case KP::AntilandRocket:
        if(ship->customFlags.contains("alrocket")) {
            if(ship->customFlags["alrocket"] == 1)
                return true;
            if(ship->customFlags["alrocket"] == -1)
                return false;
        }
        if(Utility::checkMask(ship->getId(), 0x000ff000, 0x00031000))
            return false;
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00020000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00010000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00030000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000f4000, 0x00044000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000f4000, 0x00054000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00080000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00070000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x000A0000))
            return true;
        return false;
    case KP::LandingCraft:
        if(ship->customFlags.contains("landingcraft")) {
            if(ship->customFlags["landingcraft"] == 1)
                return true;
            if(ship->customFlags["landingcraft"] == -1)
                return false;
        }
        if(Utility::checkMask(ship->getId(), 0x000f1000, 0x00021000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00080000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x000A0000))
            return true;
        return false;
    case KP::LandingTank:
        if(ship->customFlags.contains("landingtank")) {
            if(ship->customFlags["landingtank"] == 1)
                return true;
            if(ship->customFlags["landingtank"] == -1)
                return false;
        }
        if(Utility::checkMask(ship->getId(), 0x000f2000, 0x00022000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00070000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00080000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x000A0000))
            return true;
        return false;
    case KP::Drum:
        if(Utility::checkMask(ship->getId(), 0x000ff000, 0x00031000))
            return false;
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00020000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00030000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000f4000, 0x00044000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00080000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00090000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x000A0000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000ff000, 0x00011000))
            return true;
    case KP::TPMaterial: // TBD
        return false;
    case KP::EngineTurbine: [[fallthrough]];
    case KP::EngineBoiler:
        if(Utility::checkMask(ship->getId(), 0xf00ff000, 0x30010000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000ff000, 0x00010000))
            return false;
        return true; // most of ships can equip it
    case KP::SearchLight:
        if(type.getSize() == 3) {
            if(ship->customFlags.contains("searchlight")) {
                if(ship->customFlags["searchlight"] >= 3)
                    return true;
            }
            if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00050000))
                return true;
            return false;
        }
        if(ship->customFlags.contains("searchlight")) {
            if(ship->customFlags["searchlight"] > 0)
                return true;
            if(ship->customFlags["searchlight"] == -1)
                return false;
        }
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00020000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00030000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00040000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00050000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00080000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000ff000, 0x00011000))
            return true;
        return false;
    case KP::Starshell:
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00020000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00030000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00040000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00050000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00080000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x000B0000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000ff000, 0x00011000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x00ffffff, 0x00190300))
            return true; // 宗谷
        return false;
    case KP::RepairItem:
        return true;
    case KP::UnderwayReplenish:
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00090000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x00ff0fff, 0x001A0400))
            return true; // 熊野丸
        return false;
    case KP::Food:
        return true;
    case KP::CommandFacility:
        if(ship->customFlags.contains("commandfac")) {
            if(ship->customFlags["commandfac"] > 0)
                return true;
            if(ship->customFlags["commandfac"] == -1)
                return false;
        }
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00030000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00040000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00050000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00060000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00080000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x000A0000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000f8000, 0x00028000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x00ff0f00, 0x00120B00))
            return true; // 秋月型
        return false;
    case KP::AircraftPersonnel:
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x000C0000)) {
            return true;
        }
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00060000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000f4000, 0x00054000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000f4000, 0x00044000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000f4000, 0x00034000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00080000))
            return true;
        if(Utility::checkMask(ship->getId(), 0xffffffff, 0x0A190300))
            return false; // 宗谷
        if(Utility::checkMask(ship->getId(), 0xffffffff, 0x0B190300))
            return false; // 宗谷
        if(Utility::checkMask(ship->getId(), 0x00ff0fff, 0x00190600))
            return false; // 大泊
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00090000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x00ff0fff, 0x001A0200))
            return true; // 神州丸
        return false;
    case KP::RepairFacility:
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x000B0000))
            return true;
        if(Utility::checkMask(ship->getId(), 0xffffffff, 0x20180501))
            return true; // 秋津洲改
        return false;
    case KP::SurfacePersonnel:
        if(ship->customFlags.contains("lookout")) {
            if(ship->customFlags["lookout"] == 1)
                return true;
            if(ship->customFlags["lookout"] == -1)
                return false;
        }
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00020000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00030000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00040000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00050000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00080000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000ff000, 0x00010000))
            return true;
        return false;
    case KP::AntiAir:
        return true;
    case KP::FlyingBoat:
        if(ship->customFlags.contains("flyingboat")) {
            if(ship->customFlags["flyingboat"] == 1)
                return true;
            if(ship->customFlags["flyingboat"] == -1)
                return false;
        }
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x000C0000)) {
            return true;
        }
        return false;
    case KP::LBInterceptor:
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x000C0000)) {
            return true;
        }
        return false;
    case KP::JetPlane:
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x000C0000)) {
            return true;
        }
        if(type.isRecon()) {
            return Utility::checkMask(ship->getId(), 0x000f4000, 0x00064000);
        }
        if(type.isFighter() && type.isDiveBomber()) {
            if(ship->customFlags.contains("jet")) {
                if(ship->customFlags["jet"] == 1)
                    return true;
                if(ship->customFlags["jet"] == -1)
                    return false;
            }
        }
        return false;
    case KP::Bulge:
        if(type.getSize() == 1) {
            return Utility::checkMask(ship->getId(), 0x000f4000, 0x00024000);
        }
        if(type.getSize() == 2) {
            if(getId() == 268) {
                if(Utility::checkMask(ship->getId(), 0x00f00000, 0x00700000))
                    return true; // All Soviets
                if(Utility::checkMask(ship->getId(), 0x00f00000, 0x00A00000))
                    return true; // All Nordics
                if(ship->customFlags.contains("bulge")) {
                    if(ship->customFlags["bulge"] & 0x8)
                        return true; // 大泊、多摩(未改装)、木曾(未改装)、阿武隈(全段階)
                }
            }
            if(ship->customFlags.contains("bulge")) {
                if(ship->customFlags["bulge"] & 0x2)
                    return true;
            }
            if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00040000))
                return true;
            if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00061000))
                return true;
            if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00080000))
                return true;
            if(Utility::checkMask(ship->getId(), 0x000f1000, 0x00031000))
                return true;
            if(Utility::checkMask(ship->getId(), 0x000f0000, 0x000B0000))
                return true;
            return false;
        }
        if(type.getSize() == 3) {
            if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00050000))
                return true;
            if(Utility::checkMask(ship->getId(), 0x000f1000, 0x00060000))
                return true;
            if(ship->customFlags.contains("bulge")) {
                if(ship->customFlags["bulge"] & 0x4)
                    return true;
            }
            return false;
        }
        return false;
    case KP::AAControl:
        if(Utility::checkMask(ship->getId(), 0x00ff0fff, 0x00190600))
            return false; // 大泊
        if(Utility::checkMask(ship->getId(), 0x00ff0fff, 0x001A0301))
            return false; // 第百一号輸送艦
        if(Utility::checkMask(ship->getId(), 0xffff0fff, 0x101A0200))
            return false; // 未改造神州丸
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00070000))
            return false;
        return true;
    case KP::LandCorps:
        return Utility::checkMask(ship->getId(), 0x00ff0fff, 0x001A0301); // 第百一号輸送艦
    default:
        return false;
    }
}

bool Equipment::canEquipEX(Ship *ship) const
{
    switch(type.getSpecial())
    {
    case KP::RepairItem: return true;
    case KP::Food: return true;
    case KP::UnderwayReplenish: return canEquip(ship);
    case KP::AntilandShell:
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00040000))
            return true;
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00050000))
            return true;
        return false;
    case KP::NonSpecial:
        if(type.isSecGun()) {
            if(getId() == 66 || getId() == 220) {
                /* 8cm */
                if(ship->customFlags.contains("secgunex")) {
                    return ship->customFlags["secgunex"] & 0x1;
                }
                else {
                    if(Utility::checkMask(ship->getId(), 0x000f1000, 0x00031000))
                        return true;
                    if(Utility::checkMask(ship->getId(), 0x000f0000, 0x000B0000))
                        return true;
                }
                return false;
            }
            if(getId() == 71 || getId() == 275) {
                /* 10cm連装高角砲(砲架) 系列 */
                if(ship->customFlags.contains("secgunex")) {
                    return ship->customFlags["secgunex"] & 0x2;
                }
                return false;
            }
            if(getId() == 464) {
                /* 10cm連装高角砲群 集中配備 */
                if(ship->customFlags.contains("secgunex")) {
                    return ship->customFlags["secgunex"] & 0x4;
                }
                return false;
            }
            if(getId() == 524) {
                /* 12cm単装高角砲＋25mm機銃増備 */
                if(ship->customFlags.contains("secgunex")) {
                    return ship->customFlags["secgunex"] & 0x8;
                }
                else {
                    if(Utility::checkMask(ship->getId(), 0x000f1000, 0x00031000))
                        return true;
                    if(Utility::checkMask(ship->getId(), 0x000f0000, 0x000B0000))
                        return true;
                }
                return false;
            }
            if(getId() == 10 || getId() == 130) {
                /* 12.7cm連装高角砲 系列 */
                if(ship->customFlags.contains("secgunex")) {
                    return ship->customFlags["secgunex"] & 0x10;
                }
                return false;
            }
            if(getId() == 12 || getId() == 234 || getId() == 463) {
                /* 15.5cm三連装副砲 系列 */
                if(ship->customFlags.contains("secgunex")) {
                    return ship->customFlags["secgunex"] & 0x20;
                }
                return false;
            }
            return false;
        }
        else if(type.isRadar()) {
            if(type.getSize() == 7) {
                return canEquip(ship);
            }
            if(getId() == 27 || getId() == 106 || getId() == 450) {
                /* 13号対空電探 系列 */
                if(ship->customFlags.contains("radarex")) {
                    return ship->customFlags["radarex"] & 0x1;
                }
                else {
                    if(Utility::checkMask(ship->getId(), 0x00ff0f00, 0x00130600))
                        return true; // 阿賀野型
                    if(Utility::checkMask(ship->getId(), 0x00ff0f00, 0x00130700))
                        return true; // 大淀型
                    if(Utility::checkMask(ship->getId(), 0x00ff0f00, 0x00120900))
                        return true; // 夕雲型
                    if(Utility::checkMask(ship->getId(), 0x00ff0f00, 0x00120B00))
                        return true; // 秋月型
                    if(Utility::checkMask(ship->getId(), 0x00ff0f00, 0x00120C00))
                        return true; // 松型
                }
                return false;
            }
            if(getId() == 28 || getId() == 88 || getId() == 240 || getId() == 517) {
                /* 22号対水上電探 系列 */
                if(ship->customFlags.contains("radarex")) {
                    return ship->customFlags["radarex"] & 0x2;
                }
                else {
                    if(Utility::checkMask(ship->getId(), 0x00ff0f00, 0x00120800))
                        return true; // 陽炎型
                    if(Utility::checkMask(ship->getId(), 0x00ff0f00, 0x00120900))
                        return true; // 夕雲型
                    if(Utility::checkMask(ship->getId(), 0x00ff0f00, 0x00120C00))
                        return true; // 松型
                }
                return false;
            }
            if(getId() == 506) {
                /* 電探装備マスト(13号改＋22号電探改四) */
                if(ship->customFlags.contains("radarex")) {
                    return ship->customFlags["radarex"] & 0x4;
                }
                return false;
            }
            if(getId() == 410 || getId() == 411) {
                /* 21号対空電探改二 42号対空電探改二 */
                if(ship->customFlags.contains("radarex")) {
                    return ship->customFlags["radarex"] & 0x8;
                }
                return false;
            }
            if(getId() == 527) {
                /* Type281 レーダー */
                if(Utility::checkMask(ship->getId(), 0x00ff0000, 0x00530000))
                    return true;
                if(Utility::checkMask(ship->getId(), 0x00ff0000, 0x00560000))
                    return true;
                if(Utility::checkMask(ship->getId(), 0x00ff0000, 0x00550000))
                    return true;
                return false;
            }
            if(getId() == 528) {
                /* Type274 射撃管制レーダー */
                if(Utility::checkMask(ship->getId(), 0x00ff0000, 0x00530000))
                    return true;
                if(Utility::checkMask(ship->getId(), 0x00ff0000, 0x00550000))
                    return true;
                return false;
            }
            if(getId() == 124) {
                /* FuMO25 レーダー */
                if(Utility::checkMask(ship->getId(), 0xf0000000, 0x10000000))
                    return false;
                if(Utility::checkMask(ship->getId(), 0x00ff0000, 0x00250000))
                    return true;
                if(Utility::checkMask(ship->getId(), 0x00ff0000, 0x00240000))
                    return true;
                return false;
            }
            if(type.getSize() == 3) {
                if(Utility::checkMask(ship->getId(), 0xffff8000, 0x30158000))
                    return true; // 大和改二、武蔵改二
                return false;
            }
            return false;
        }
        else if(type.isTorp()) {
            if(getId() == 442 || getId() == 443) {
                return Utility::checkMask(ship->getId(), 0x000f0000, 0x00070000);
            }
            return false;
        }
        return false;
    case KP::AntiAir:
        return canEquip(ship);
    case KP::DepthCharge:
        if(type.getSize() == 2) {
            return false;
        }
        else if(type.getSize() == 1) {
            if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00010000))
                return true;
            if(Utility::checkMask(ship->getId(), 0xffffffff, 0x40126602))
                return true; // 时雨改三
        }
        return false;
    case KP::LandingCraft:
        if(getId() == 408) {
            if(Utility::checkMask(ship->getId(), 0x00ff0fff, 0x001A0200))
                return true; // 神州丸
        }
        return false;
    case KP::LandingTank:
        if(getId() == 525 || getId() == 526) {
            if(ship->customFlags.contains("toku4")) {
                if(ship->customFlags["toku4"] == 1)
                    return true;
                if(ship->customFlags["toku4"] == -1)
                    return false;
            }
        }
        return false;
    case KP::SurfacePersonnel:
        return canEquip(ship);
    case KP::AircraftPersonnel:
        if(Utility::checkMask(ship->getId(), 0x000f0000, 0x00060000))
            return canEquip(ship);
    case KP::EngineTurbine:
        return canEquip(ship);
    case KP::EngineBoiler:
        return ship->attr["Speed"] >= 41;
    case KP::Bulge:
        return canEquip(ship);
    case KP::CommandFacility:
        if(getId() == 413) {
            if(Utility::checkMask(ship->getId(), 0x00ff0f00, 0x00130200)) {
                return canEquip(ship);
            }
            if(Utility::checkMask(ship->getId(), 0x00ff0f00, 0x00130300)) {
                return canEquip(ship);
            }
            if(Utility::checkMask(ship->getId(), 0x00ff0f00, 0x00130400)) {
                return canEquip(ship);
            }
            if(Utility::checkMask(ship->getId(), 0x00ff0f00, 0x00130600)) {
                return canEquip(ship);
            }
            if(Utility::checkMask(ship->getId(), 0x00ff0f00, 0x00130700)) {
                return canEquip(ship);
            }
            if(Utility::checkMask(ship->getId(), 0x00ff0f00, 0x00120900)) {
                return canEquip(ship);
            }
            if(Utility::checkMask(ship->getId(), 0x00ff0f00, 0x00120B00)) {
                return canEquip(ship);
            }
        }
        return false;
    default:
        return false;
    }
}
