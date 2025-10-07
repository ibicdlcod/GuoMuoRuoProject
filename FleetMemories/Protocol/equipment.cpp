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
    return localNames[lang];
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
                return !Utility::checkMask(ship->getId(), 0x000f0000, 0x00070000);
            case 2:
            {
                if(ship->customFlags.contains("bigradar")) {
                    if(ship->customFlags["bigradar"] == 1)
                        return true;
                    if(ship->customFlags["bigradar"] == -1)
                        return false;
                }
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
            if(type.getSize() == 1 && Utility::checkMask(ship->getId(), 0x000ff000, 0x00074000)) {
                return true;
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
            }
        }
        else if(type.isSecGun()) {
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
        else if(type.isMainGun()){
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
            case 3:
            case 4:
            case 5:
            case 6:
            default: return false;
            }

        }
        return false;
    case KP::MidgetSub:
    case KP::DepthCharge:
    case KP::Smoke:
    case KP::Sonar:
    case KP::Ballon:
    case KP::APShell:
    case KP::AntilandShell:
    case KP::AntilandRocket:
    case KP::LandingCraft:
    case KP::LandingTank:
    case KP::Drum:
    case KP::TPMaterial:
    case KP::EngineTurbine:
    case KP::EngineBoiler:
    case KP::SearchLight:
    case KP::Starshell:
    case KP::RepairItem:
    case KP::UnderwayReplenish:
    case KP::Food:
    case KP::CommandFacility:
    case KP::AircraftPersonnel:
    case KP::RepairFacility:
    case KP::SurfacePersonnel:
    case KP::AntiAir:
    case KP::FlyingBoat:
    case KP::LBInterceptor:
    case KP::JetPlane:
    case KP::Bulge:
    case KP::AAControl:
    case KP::LandCorps:
    default:
        return false;
    }
}

bool Equipment::canEquipEX(Ship *ship) const
{
    return false;
}
