/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "ship.h"
#include <QSettings>
#include <QSqlQuery>
#include "tech.h"
#include "../Server/kerrors.h"
#include "utility.h"

Ship::Ship(int shipId, QObject *parent)
    : shipRegId(shipId), QObject(parent){
    if(shipId == 0) {
        return;
    }

    for(auto &lang: *KP::supportedLangs) {
        QSqlQuery query;
        query.prepare(
            "SELECT value FROM ShipName "
            "WHERE ShipID = :id AND lang = :lang AND textattr = 'name'");
        query.bindValue(":id", shipId);
        query.bindValue(":lang", lang);
        if(!query.exec() || !query.isSelect()) {
            qCritical() << query.lastQuery();
            //% "Local language (%1) for ship name not found!"
            throw DBError(qtTrId("ship-local-name-lack").arg(lang),
                          query.lastError());
        }
        else if(query.first()) {
            localNames[lang] = query.value(0).toString();
        }
    }
    for(auto &lang: *KP::supportedLangs) {
        QSqlQuery query;
        query.prepare(
            "SELECT value FROM ShipName "
            "WHERE ShipID = :id AND lang = :lang AND textattr = 'shipclasstext'");
        query.bindValue(":id", shipId);
        query.bindValue(":lang", lang);
        if(!query.exec() || !query.isSelect()) {
            qCritical() << query.lastQuery();
            //% "Local language (%1) for ship name not found!"
            throw DBError(qtTrId("ship-local-name-lack").arg(lang),
                          query.lastError());
        }
        else if(query.first()) {
            shipClassText[lang] = query.value(0).toString();
        }
    }
    for(auto &lang: *KP::supportedLangs) {
        QSqlQuery query;
        query.prepare(
            "SELECT value FROM ShipName "
            "WHERE ShipID = :id AND lang = :lang AND textattr = 'shipordertext'");
        query.bindValue(":id", shipId);
        query.bindValue(":lang", lang);
        if(!query.exec() || !query.isSelect()) {
            qCritical() << query.lastQuery();
            //% "Local language (%1) for ship name not found!"
            throw DBError(qtTrId("ship-local-name-lack").arg(lang),
                          query.lastError());
        }
        else if(query.first()) {
            shipOrderText[lang] = query.value(0).toString();
        }
    }

    QSqlQuery query2;
    query2.prepare(
        "SELECT Intvalue, Attribute FROM ShipReg "
        "WHERE ShipID = :id ");
    query2.bindValue(":id", shipId);
    if(!query2.exec() || !query2.isSelect()) {
        qCritical() << query2.lastQuery();
        //% "Fetch ship attributes failure!"
        throw DBError(qtTrId("ship-attr-lack"),
                      query2.lastError());
    }
    else {
        while(query2.next()) {
            QString attrName = query2.value(1).toString();
            if(attrName.startsWith("CUSTOM")) {
                customFlags[attrName.last(attrName.length()
                                          - QStringLiteral("CUSTOM").length())]
                    = query2.value(0).toInt();
            }
            else {
                attr[attrName] = query2.value(0).toInt();
            }
        }
    }
}

Ship::Ship(const QJsonObject &input, QObject *parent)
    : QObject(parent){
    shipRegId = input["sid"].toInt();
    if(shipRegId == 0)
        return;
    QJsonObject lNames = input["name"].toObject();
    for(auto &lang: lNames.keys()) {
        localNames[lang] =
            lNames.value(lang).toString();
    };
    QJsonObject lClassTexts = input["class"].toObject();
    for(auto &lang: lClassTexts.keys()) {
        shipClassText[lang] =
            lClassTexts.value(lang).toString();
    };
    QJsonObject lOrderTexts = input["shiporder"].toObject();
    for(auto &lang: lOrderTexts.keys()) {
        shipOrderText[lang] =
            lOrderTexts.value(lang).toString();
    };
    QJsonObject attrs = input["attr"].toObject();
    for(auto &attrI: attrs.keys()) {
        attr[attrI] =
            attrs.value(attrI).toInt();
    }
}

int Ship::operator<=>(const Ship &other) const {
    int typeResult = this->getType().getTypeSort()
    - other.getType().getTypeSort();
    if(typeResult == 0)
        return shipRegId - other.shipRegId;
    else
        return typeResult;
}

/* not operator!= because QObject don't have == */
bool Ship::isNotEqual(const Ship &other) const {
    return operator<=>(other) != 0;
}

QString Ship::toString(QString lang) const {
    return localNames[lang].isEmpty() ? localNames["ja_JP"] : localNames[lang];
}

/* 5.4-construction.md#Resource cost */
const ResOrd Ship::consRes() const {
    qint64 devResScale = settings->value("rule/devresscale", 10).toLongLong();
    return getType().consResBase() * (qint64)std::round((getTech() + 1.0)
                                                         * devResScale);
}

/* 5.4-construction.md#Construct time */
const int Ship::consTimeInSec() const {
    double ctrl = settings->value("rule/techfactorcontroller", 5.0).toDouble();
    qint64 devTimebase = getType().consTimeBase();
    qint64 devResScale = settings->value("rule/devresscale", 10).toLongLong();
    double techFactor = getTech() * getTech() / std::hypot(ctrl, getTech()) + 0.1;
    return devTimebase * (qint64)std::round(techFactor  * devResScale);
}

/* 8.2-repair.md#Resource cost */
const ResOrd Ship::repairRes() const {
    double ctrl = settings->value("rule/techfactorcontroller", 5.0).toDouble();
    double techFactor = (getTech() + 1.0) / std::hypot(ctrl, (getTech() + 1.0));
    return getType().repairResBase() * (qint64)std::round(techFactor * attr["Hitpoints"]);
}

/* 8.2-repair.md#Repair time */
/* real repair time is hp * (this * lv) / (std::hypot(1, lv/25)) */
double Ship::repairTimeInSecUnleveledPerhp() const {
    double ctrl = settings->value("rule/techfactorcontroller", 5.0).toDouble();
    double devTimebase = getType().repairTimeBase();
    double techFactor = (getTech() + 1.0) / std::hypot(ctrl, (getTech() + 1.0));
    return devTimebase * techFactor;
}

QLocale::Territory Ship::getAllegiance() const {
    if(isAmnesiac()) {
        return QLocale::AnyTerritory;
    }
    return static_cast<QLocale::Territory>(attr["Allegiance"]);
}

KP::AllegianceGroup Ship::getAllegianceGroup() const {
    return allegianceGroup(getAllegiance());
}

KP::AllegianceSubGroup Ship::getAllegianceSubGroup() const {
    if(((getId() & 0xF0) >= 0xD0) && getAllegiance() == QLocale::Japan) {
        if(Utility::checkMask(getId(), 0x00F000F0, 0x001000D0)) {
            return KP::DJapaneseOutlying;
        }
        else if(Utility::checkMask(getId(), 0x00F000F0, 0x001000E0)) {
            return KP::DJapaneseExterior;
        }
        else if(Utility::checkMask(getId(), 0x00F000F0, 0x001000F0)) {
            return KP::DRyukyuan;
        }
    }
    return allegianceSubGroup(getAllegiance());
}

KP::AllegianceGroup Ship::mapOpenRule() const {
    /* The result is geographical approximation of map.svg,
     * not ship's actual allegiance group */
    switch(getAllegianceGroup()) {
    case KP::UnknownNation: return KP::UnknownNation;
    case KP::Japanese: return KP::Japanese;
    case KP::German: return KP::German;
    case KP::Italian: return getAllegianceSubGroup()
                       == KP::DEastAfrican ? KP::British
                   : KP::Italian;
    case KP::American: return getAllegianceSubGroup()
                       == KP::DFilipino ? KP::Japanese
                   : KP::American;
    case KP::British: return KP::British;
    case KP::French: {
        switch(getAllegianceSubGroup()) {
        case KP::DFrench:
            switch(getAllegiance()) {
            case QLocale::FrenchGuiana: return KP::American;
            case QLocale::Guadeloupe: return KP::American;
            case QLocale::Martinique: return KP::American;
            case QLocale::Mayotte: return KP::British;
            case QLocale::Reunion: return KP::British;
            default: return KP::French;
            }
        case KP::DIndochinese: return KP::Japanese;
        case KP::DAlgerian: [[fallthrough]];
        case KP::DMoroccoan: [[fallthrough]];
        case KP::DTunisian: return KP::Italian;
        case KP::DMauritanian: return KP::British;
        case KP::DOtherFrancophone: {
            switch(getAllegiance()) {
            case QLocale::ClippertonIsland: return KP::American;
            case QLocale::Comoros: return KP::British;
            case QLocale::Djibouti: return KP::British;
            case QLocale::FrenchPolynesia: return KP::Commonwealth;
            case QLocale::FrenchSouthernTerritories: return KP::British;
            case QLocale::Madagascar: return KP::British;
            case QLocale::NewCaledonia: return KP::Commonwealth;
            case QLocale::SaintBarthelemy: [[fallthrough]];
            case QLocale::SaintMartin: [[fallthrough]];
            case QLocale::SaintPierreAndMiquelon: return KP::American;
            case QLocale::Seychelles: return KP::British;
            case QLocale::WallisAndFutuna: return KP::Commonwealth;
            default: return KP::French;
            }
        }
        default: return KP::French;
        }
    }
    case KP::Soviet: return KP::Soviet;
    case KP::Chinese: return KP::Japanese;
    case KP::Benelux: {
        switch(getAllegianceSubGroup()) {
        case KP::DIndonesian: return KP::Commonwealth;
        case KP::DDutch: return getAllegiance()
                           == QLocale::CaribbeanNetherlands ? KP::American
                       : KP::British;
        case KP::DDutchSpeakingAmericas: return KP::American;
        case KP::DBelgian: return KP::French;
        case KP::DCentralAfrican: return KP::French;
        case KP::DBeneluxOther: return KP::German;
        default: return KP::British;
        }
    }
    case KP::Nordic: {
        switch(getAllegianceSubGroup()) {
        case KP::DDanishKingdom: return getAllegiance()
            == QLocale::FaroeIslands ? KP::British
                                     : KP::American; // Greeland
        case KP::DNorwegian: return KP::British;
        case KP::DIcelandic: return KP::American;
        default: return KP::German;
        }
    }
    case KP::Commonwealth: {
        switch(getAllegianceSubGroup()) {
        case KP::DAustralian: [[fallthrough]];
        case KP::DNewZealander: [[fallthrough]];
        case KP::DOceanaian: return KP::Commonwealth;
        case KP::DMalaysianOrBruneian: return KP::Japanese;
        case KP::DSingaporean: return KP::Japanese;
        case KP::DCanadian: return KP::American;
        case KP::DOtherCommonwealth: {
            switch(getAllegiance()) {
            case QLocale::Anguilla: [[fallthrough]];
            case QLocale::AntiguaAndBarbuda: [[fallthrough]];
            case QLocale::Bahamas: [[fallthrough]];
            case QLocale::Barbados: [[fallthrough]];
            case QLocale::Belize: [[fallthrough]];
            case QLocale::Bermuda: [[fallthrough]];
            case QLocale::BritishVirginIslands: [[fallthrough]];
            case QLocale::CaymanIslands: [[fallthrough]];
            case QLocale::Dominica: [[fallthrough]];
            case QLocale::Grenada: [[fallthrough]];
            case QLocale::Guyana: [[fallthrough]];
            case QLocale::Jamaica: return KP::American;
            case QLocale::Malta: return KP::Italian;
            case QLocale::Montserrat: [[fallthrough]];
            case QLocale::SaintKittsAndNevis: [[fallthrough]];
            case QLocale::SaintLucia: [[fallthrough]];
            case QLocale::SaintVincentAndGrenadines: [[fallthrough]];
            case QLocale::TurksAndCaicosIslands: return KP::American;
            default: return KP::British;
            }
        }
        default: return KP::British;
        }
    }
    case KP::Latino: {
        switch(getAllegianceSubGroup()) {
        case KP::DSpanish: [[fallthrough]];
        case KP::DPortuguese: return KP::British;
        case KP::DOtherLatino: {
            switch(getAllegiance()) {
            case QLocale::TimorLeste: return KP::Commonwealth;
            default: return KP::British;
            }
        }
        default: return KP::American;
        }
    }
    case KP::EasternEuropean: {
        switch(getAllegianceSubGroup()) {
        case KP::DPolish: return KP::German;
        case KP::DBulgarian: return KP::Soviet;
        case KP::DRomanian: return KP::Soviet;
        case KP::DBaltic: return KP::German;
        default: return KP::Italian;
        }
    }
    case KP::MinorAsian: {
        switch(getAllegianceSubGroup()) {
        case KP::DIranian: return KP::Soviet;
        case KP::DArabicAsian: return KP::British;
        default: return KP::Japanese;
        }
    }
    case KP::Fantasy: return KP::UnknownNation;
    default: return KP::UnknownNation;
    }
    return KP::UnknownNation;
}

int Ship::getId() const {
    return shipRegId;
}

QList<int> Ship::getLaterModels(const QMap<int, Ship *> &registry) const {
    QList<int> result;
    int current = shipRegId;
    int later = -1;
    while(true) {
        later = registry[current]->attr["remodel"];
        if(later == 0) {
            break;
        }
        if(!registry.contains(later)) {
            //% "Remodel target %1 does not exist!"
            qCritical() << qtTrId("remodel-nonexistent").arg(later);
            break;
        }
        if(result.contains(later)) {
            break;
        }
        else {
            result.append(later);
            current = later;
        }
    }
    return result;
}

QList<int> Ship::getPreviousModels(const QMap<int, Ship *> &registry) const {
    QList<int> result;
    for(auto [id, candidate]: registry.asKeyValueRange()) {
        int remodel = candidate->attr["remodel"];
        if(remodel == getId()) {
            result.append(id);
        }
    }
    return result;
}

KP::AllegianceGroup Ship::getNationality() const {
    return getAllegianceGroup();
    //return static_cast<KP::AllegianceGroup>((shipRegId & 0x00F00000) >> 20);
}

QList<int> Ship::getStartingEquip() const {
    QList<int> result;
    for(int i = 1; i <= 5; ++i) {
        QString attrId = "Defaultequip" + QString::number(i);
        if(attr.contains(attrId) && attr[attrId] != 0) {
            result.append(attr[attrId]);
        }
    }
    return result;
}

double Ship::getTech() const {
    return Tech::techYearToCompact(attr["Tech"]);
}

ShipType Ship::getType() const {
    return ShipType(shipRegId);
}

QList<std::tuple<int, int>> Ship::getVisibleBonuses() const {
    /* TODO: this is temporary */
    QList<std::tuple<int, int>> result;
    for(const auto &defaultequip: getStartingEquip()) {
        result.append({defaultequip, 1});
    }
    return result;
}

bool Ship::isAmnesiac() const {
    return Utility::checkMask(shipRegId, 0xF0000000, 0x70000000);
}

int Ship::getLevel(int exp) {
    /* inverse of y / 100 = (x)(x-1)/2 */
    return std::floor(
        (1.0 + sqrt(1.0 + 8.0 * (exp /
                                 settings->value("rule/shipexpscale", 100.0)
                                     .toDouble())))/ 2.0);
}

double Ship::getEfficiency(int lv, int star) {
    double modernizationFactor =
        star / std::hypot(settings->value("rule/equipmentstandardstar", 10.0)
                              .toDouble(), star);
    return 0.5 + std::sqrt(0.5) * ((double)lv / std::hypot(lv, ringLv))
           + (1 - std::sqrt(0.5)) * modernizationFactor;
}

int Ship::expCap(int numberOfRings) {
    int levelCap = ringLv * (numberOfRings + 1);
    return (settings->value("rule/shipexpscale", 100.0).toDouble()
            + (levelCap - 1) * settings->value("rule/shipexpscale", 100.0).toDouble())
           / 2 * (levelCap - 1);
}

KP::AllegianceSubGroup Ship::allegianceSubGroup(QLocale::Territory ter) {
    /* The World War II involved not only the great powers but the people of subjugated colonies and practically subjugated countries.
     * The great contribution or struggle of them subsequently made the colonial system became generally unviable and mostly replaced
     * by international organizations based on lingua franca. To offer respect to them, all countries/territories (that Qt recognizes)
     * is included in this function, even if they are landlocked, or not independent during WWII, or have no navy to speak of even today.
     *
     * If a great power's ship is primarily deployed to colonies during WWII, its nationality will count as that of said colony here.
     * An example is De Ruyter, who is regarded as Dutch East Indian and dedicated to Indonesian people in this game, rather than Dutch.
     *
     * The below "nationality sub-groups" is primarily determined by status during WWII rather than modern status. In paticular, the
     * first hex digit "nationality group" is even more heavily based on WWII, regardless of how little these countries are connected
     * in modern times. Exceptions are made, such as South Korea having a significant navy in modern times, so Korea's space (0xE8-0xEB)
     * is not lumped with Japan's (0x10-0x1F), which also have a large navy, despite the fact Korea was Japan's colony during WWII.
     *
     * It's useless to pretend that no political points are made in this function; disagreers are encouraged to fork this project
     * instead of complaining. */

    /* Dependencies are treated not part of its overlord unless fully incorporated; instances of that are noted below */
    switch(ter) {
    case QLocale::AnyTerritory: return KP::DUnknownNation;
    case QLocale::Afghanistan: return KP::DOtherAsian;
    case QLocale::AlandIslands: return KP::DFinnish;
    case QLocale::Albania: return KP::DAlbanian;
    case QLocale::Algeria: return KP::DAlgerian;
    case QLocale::AmericanSamoa: return KP::DAmericanAssociates;
    case QLocale::Andorra: return KP::DOtherEuropean;
    case QLocale::Angola: return KP::DOtherLatino;
    case QLocale::Anguilla: return KP::DOtherCommonwealth;
    case QLocale::Antarctica: return KP::DUnknownNation;
    case QLocale::AntiguaAndBarbuda: return KP::DOtherCommonwealth;
    case QLocale::Argentina: return KP::DArgentinian;
    case QLocale::Armenia: return KP::DCIS;
    case QLocale::Aruba: return KP::DDutchSpeakingAmericas;
    case QLocale::AscensionIsland: return KP::DOtherCommonwealth;
    case QLocale::Australia: return KP::DAustralian;
    case QLocale::Austria: return KP::DAustrian;
    case QLocale::Azerbaijan: return KP::DCIS;
    case QLocale::Bahamas: return KP::DOtherCommonwealth;
    case QLocale::Bahrain: return KP::DArabicAsian;
    case QLocale::Bangladesh: return KP::DBangladeshi;
    case QLocale::Barbados: return KP::DOtherCommonwealth;
    case QLocale::Belarus: return KP::DCIS;
    case QLocale::Belgium: return KP::DBelgian;
    case QLocale::Belize: return KP::DOtherCommonwealth;
    case QLocale::Benin: return KP::DOtherFrancophone;
    case QLocale::Bermuda: return KP::DOtherCommonwealth;
    case QLocale::Bhutan: return KP::DOtherAsian;
    case QLocale::Bolivia: return KP::DOtherLatinAmerican;
    case QLocale::BosniaAndHerzegovina: return KP::DYugoslavian;
    case QLocale::Botswana: return KP::DOtherCommonwealth;
    case QLocale::BouvetIsland: return KP::DNorwegian;
    case QLocale::Brazil: return KP::DBrazilian;
    case QLocale::BritishIndianOceanTerritory: return KP::DOtherCommonwealth;
    case QLocale::BritishVirginIslands: return KP::DOtherCommonwealth;
    case QLocale::Brunei: return KP::DMalaysianOrBruneian;
    case QLocale::Bulgaria: return KP::DBulgarian;
    case QLocale::BurkinaFaso: return KP::DOtherFrancophone;
    case QLocale::Burundi: return KP::DCentralAfrican;
    case QLocale::Cambodia: return KP::DIndochinese;
    case QLocale::Cameroon: return KP::DOtherFrancophone;
    case QLocale::Canada: return KP::DCanadian;
    case QLocale::CanaryIslands: return KP::DSpanish;
    /* This is a fully integrated part of Netherlands */
    case QLocale::CaribbeanNetherlands: return KP::DDutch;
    case QLocale::CapeVerde: return KP::DOtherLatino;
    case QLocale::CaymanIslands: return KP::DOtherCommonwealth;
    case QLocale::CentralAfricanRepublic: return KP::DOtherFrancophone;
    case QLocale::CeutaAndMelilla: return KP::DSpanish;
    case QLocale::Chad: return KP::DOtherFrancophone;
    case QLocale::Chile: return KP::DChilean;
    case QLocale::China: return KP::DChineseModern;
    case QLocale::ChristmasIsland: return KP::DOceanaian;
    case QLocale::ClippertonIsland: return KP::DOtherFrancophone;
    case QLocale::CocosIslands: return KP::DOceanaian;
    case QLocale::Colombia: return KP::DColumbianOrEcuadoran;
    case QLocale::Comoros: return KP::DOtherFrancophone;
    case QLocale::CongoBrazzaville: return KP::DOtherFrancophone;
    case QLocale::CongoKinshasa: return KP::DCentralAfrican;
    case QLocale::CookIslands: return KP::DOceanaian;
    case QLocale::CostaRica: return KP::DOtherLatinAmerican;
    case QLocale::Croatia: return KP::DYugoslavian;
    case QLocale::Cuba: return KP::DCuban;
    case QLocale::Curacao: return KP::DDutchSpeakingAmericas;
    case QLocale::Cyprus: return KP::DGreekOrCypriot;
    case QLocale::Czechia: return KP::DOtherEuropean;
    case QLocale::Denmark: return KP::DDanish;
    case QLocale::DiegoGarcia: return KP::DOtherCommonwealth;
    case QLocale::Djibouti: return KP::DOtherFrancophone;
    case QLocale::Dominica: return KP::DOtherCommonwealth;
    case QLocale::DominicanRepublic: return KP::DOtherLatinAmerican;
    case QLocale::Ecuador: return KP::DColumbianOrEcuadoran;
    case QLocale::Egypt: return KP::DEgyptian;
    case QLocale::ElSalvador: return KP::DOtherLatinAmerican;
    case QLocale::EquatorialGuinea: return KP::DOtherLatino;
    case QLocale::Eritrea: return KP::DEastAfrican;
    case QLocale::Estonia: return KP::DBaltic;
    case QLocale::Eswatini: return KP::DOtherCommonwealth;
    /* assigned this for our focus is WWII */
    case QLocale::Ethiopia: return KP::DEastAfrican;
    case QLocale::EuropeanUnion: return KP::DOtherEuropean;
    case QLocale::Europe: return KP::DOtherEuropean;
    /* just use QLocale::UnitedKingdom->DBritish for actual British ships that involved Falkland */
    case QLocale::FalklandIslands: return KP::DArgentinian;
    case QLocale::FaroeIslands: return KP::DDanishKingdom;
    case QLocale::Fiji: return KP::DOceanaian;
    case QLocale::Finland: return KP::DFinnish;
    case QLocale::France: return KP::DFrench;
    /* This is a fully integrated overseas department of France */
    case QLocale::FrenchGuiana: return KP::DFrench;
    case QLocale::FrenchPolynesia: return KP::DOtherFrancophone;
    case QLocale::FrenchSouthernTerritories: return KP::DOtherFrancophone;
    case QLocale::Gabon: return KP::DOtherFrancophone;
    case QLocale::Gambia: return KP::DOtherCommonwealth;
    case QLocale::Georgia: return KP::DCIS;
    case QLocale::Germany: return KP::DGerman;
    case QLocale::Ghana: return KP::DOtherCommonwealth;
    /* just use QLocale::UnitedKingdom->DBritish for actual British ships that involved Gibraltar */
    case QLocale::Gibraltar: return KP::DOtherEuropean;
    case QLocale::Greece: return KP::DGreekOrCypriot;
    case QLocale::Greenland: return KP::DDanishKingdom;
    case QLocale::Grenada: return KP::DOtherCommonwealth;
    /* This is a fully integrated overseas department of France */
    case QLocale::Guadeloupe: return KP::DFrench;
    case QLocale::Guam: return KP::DAmericanAssociates;
    case QLocale::Guatemala: return KP::DOtherLatinAmerican;
    case QLocale::Guernsey: return KP::DOtherCommonwealth;
    case QLocale::Guinea: return KP::DOtherFrancophone;
    case QLocale::GuineaBissau: return KP::DOtherLatino;
    case QLocale::Guyana: return KP::DOtherCommonwealth;
    case QLocale::Haiti: return KP::DOtherLatinAmerican;
    case QLocale::HeardAndMcDonaldIslands: return KP::DOceanaian;
    case QLocale::Honduras: return KP::DOtherLatinAmerican;
    /* just use QLocale::China->DChineseModern after 1997 */
    case QLocale::HongKong: return KP::DChineseOther;
    case QLocale::Hungary: return KP::DOtherEuropean;
    case QLocale::Iceland: return KP::DIcelandic;
    case QLocale::India: return KP::DIndian;
    case QLocale::Indonesia: return KP::DIndonesian;
    case QLocale::Iran: return KP::DIranian;
    case QLocale::Iraq: return KP::DArabicAsian;
    case QLocale::Ireland: return KP::DIrish;
    case QLocale::IsleOfMan: return KP::DOtherCommonwealth;
    case QLocale::Israel: return KP::DIsraeli;
    case QLocale::Italy: return KP::DItalian;
    case QLocale::IvoryCoast: return KP::DOtherFrancophone;
    case QLocale::Jamaica: return KP::DOtherCommonwealth;
    case QLocale::Japan: return KP::DJapanese;
    case QLocale::Jersey: return KP::DOtherCommonwealth;
    case QLocale::Jordan: return KP::DArabicAsian;
    case QLocale::Kazakhstan: return KP::DCIS;
    case QLocale::Kenya: return KP::DOtherCommonwealth;
    case QLocale::Kiribati: return KP::DOceanaian;
    /* Kosovo: see "S" section below */
    case QLocale::Kuwait: return KP::DArabicAsian;
    case QLocale::Kyrgyzstan: return KP::DCIS;
    case QLocale::Laos: return KP::DIndochinese;
    case QLocale::LatinAmerica: return KP::DOtherLatinAmerican;
    case QLocale::Latvia: return KP::DBaltic;
    case QLocale::Lebanon: return KP::DArabicAsian;
    case QLocale::Lesotho: return KP::DOtherCommonwealth;
    case QLocale::Liberia: return KP::DAmericanAssociates;
    case QLocale::Libya: return KP::DLibyan;
    case QLocale::Liechtenstein: return KP::DOtherEuropean;
    case QLocale::Lithuania: return KP::DBaltic;
    case QLocale::Luxembourg: return KP::DBeneluxOther;
    /* just use QLocale::China->DChineseModern after 1999 */
    case QLocale::Macao: return KP::DChineseOther;
    case QLocale::Macedonia: return KP::DYugoslavian;
    case QLocale::Madagascar: return KP::DOtherFrancophone;
    case QLocale::Malawi: return KP::DOtherCommonwealth;
    case QLocale::Malaysia: return KP::DMalaysianOrBruneian;
    case QLocale::Maldives: return KP::DOtherCommonwealth;
    case QLocale::Mali: return KP::DOtherFrancophone;
    case QLocale::Malta: return KP::DOtherCommonwealth;
    /* This country have COFA with United States */
    case QLocale::MarshallIslands: return KP::DAmericanAssociates;
    /* This is a fully integrated overseas department of France */
    case QLocale::Martinique: return KP::DFrench;
    case QLocale::Mauritania: return KP::DMauritanian;
    case QLocale::Mauritius: return KP::DOtherCommonwealth;
    /* This is a fully integrated overseas department of France */
    case QLocale::Mayotte: return KP::DFrench;
    case QLocale::Mexico: return KP::DMexican;
    /* This country have COFA with United States */
    case QLocale::Micronesia: return KP::DAmericanAssociates;
    case QLocale::Moldova: return KP::DCIS;
    case QLocale::Monaco: return KP::DOtherFrancophone;
    case QLocale::Mongolia: return KP::DMongolian;
    case QLocale::Montenegro: return KP::DYugoslavian;
    case QLocale::Montserrat: return KP::DOtherCommonwealth;
    case QLocale::Morocco: return KP::DMoroccoan;
    case QLocale::Mozambique: return KP::DOtherLatino;
    case QLocale::Myanmar: return KP::DOtherAsian;
    case QLocale::Namibia: return KP::DSouthAfricanOrNamibian;
    case QLocale::NauruTerritory: return KP::DOceanaian;
    case QLocale::Nepal: return KP::DOtherAsian;
    case QLocale::Netherlands: return KP::DDutch;
    case QLocale::NewCaledonia: return KP::DOtherFrancophone;
    case QLocale::NewZealand: return KP::DNewZealander;
    case QLocale::Nicaragua: return KP::DOtherLatinAmerican;
    case QLocale::Niger: return KP::DOtherFrancophone;
    case QLocale::Nigeria: return KP::DOtherCommonwealth;
    case QLocale::Niue: return KP::DOceanaian;
    case QLocale::NorfolkIsland: return KP::DAustralian;
    case QLocale::NorthernMarianaIslands: return KP::DAmericanAssociates;
    case QLocale::NorthKorea: return KP::DNorthKorean;
    case QLocale::Norway: return KP::DNorwegian;
    case QLocale::Oman: return KP::DArabicAsian;
    case QLocale::OutlyingOceania: return KP::DOceanaian;
    case QLocale::Pakistan: return KP::DPakistani;        
    /* This country have COFA with United States */
    case QLocale::Palau: return KP::DAmericanAssociates;
    case QLocale::PalestinianTerritories: return KP::DArabicAsian;
    case QLocale::Panama: return KP::DOtherLatinAmerican;
    case QLocale::PapuaNewGuinea: return KP::DOceanaian;
    case QLocale::Paraguay: return KP::DOtherLatinAmerican;
    case QLocale::Peru: return KP::DPeruvian;
    case QLocale::Philippines: return KP::DFilipino;
    case QLocale::Pitcairn: return KP::DOceanaian;
    case QLocale::Poland: return KP::DPolish;
    case QLocale::Portugal: return KP::DPortuguese;
    case QLocale::PuertoRico: return KP::DAmericanAssociates;
    case QLocale::Qatar: return KP::DArabicAsian;
    /* This is a fully integrated overseas department of France */
    case QLocale::Reunion: return KP::DFrench;
    case QLocale::Romania: return KP::DRomanian;
    /* Soviet ships that are named after Baltic states cities also goes here,
     * as they would belong to DBaltic->EasternEuropean otherwise */
    case QLocale::Russia: return KP::DSovietOrRussian;
    case QLocale::Rwanda: return KP::DCentralAfrican;
    case QLocale::SaintBarthelemy: return KP::DOtherFrancophone;
    case QLocale::SaintHelena: return KP::DOtherCommonwealth;
    case QLocale::SaintKittsAndNevis: return KP::DOtherCommonwealth;
    case QLocale::SaintLucia: return KP::DOtherCommonwealth;
    case QLocale::SaintMartin: return KP::DOtherFrancophone;
    case QLocale::SaintPierreAndMiquelon: return KP::DOtherFrancophone;
    case QLocale::SaintVincentAndGrenadines: return KP::DOtherCommonwealth;
    case QLocale::Samoa: return KP::DOceanaian;
    /* done for linguistic reasons */
    case QLocale::SanMarino: return KP::DItalian;
    case QLocale::SaoTomeAndPrincipe: return KP::DOtherLatino;
    case QLocale::SaudiArabia: return KP::DArabicAsian;
    case QLocale::Senegal: return KP::DOtherFrancophone;
    case QLocale::Kosovo: [[fallthrough]];
    case QLocale::Serbia: return KP::DYugoslavian;
    case QLocale::Seychelles: return KP::DOtherFrancophone;
    case QLocale::SierraLeone: return KP::DOtherCommonwealth;
    case QLocale::Singapore: return KP::DSingaporean;
    case QLocale::SintMaarten: return KP::DDutchSpeakingAmericas;
    case QLocale::Slovakia: return KP::DOtherEuropean;
    case QLocale::Slovenia: return KP::DYugoslavian;
    case QLocale::SolomonIslands: return KP::DOceanaian;
    case QLocale::Somalia: return KP::DEastAfrican;
    case QLocale::SouthAfrica: return KP::DSouthAfricanOrNamibian;
    case QLocale::SouthGeorgiaAndSouthSandwichIslands: return KP::DOtherCommonwealth;
    case QLocale::SouthKorea: return KP::DSouthKorean;
    case QLocale::SouthSudan: return KP::DOtherCommonwealth;
    case QLocale::Spain: return KP::DSpanish;
    case QLocale::SriLanka: return KP::DOtherCommonwealth;
    case QLocale::Sudan: return KP::DOtherCommonwealth;
    case QLocale::Suriname: return KP::DDutchSpeakingAmericas;
    case QLocale::SvalbardAndJanMayen: return KP::DNorwegian;
    case QLocale::Sweden: return KP::DSwedish;
    case QLocale::Switzerland: return KP::DOtherEuropean;
    case QLocale::Syria: return KP::DArabicAsian;
    case QLocale::Taiwan: return KP::DChineseNationalist;
    case QLocale::Tajikistan: return KP::DCIS;
    case QLocale::Tanzania: return KP::DOtherCommonwealth;
    case QLocale::Thailand: return KP::DThai;
    case QLocale::TimorLeste: return KP::DOtherLatino;
    case QLocale::Togo: return KP::DOtherFrancophone;
    case QLocale::TokelauTerritory: return KP::DOceanaian;
    case QLocale::Tonga: return KP::DOceanaian;
    case QLocale::TrinidadAndTobago: return KP::DOtherLatinAmerican;
    case QLocale::TristanDaCunha: return KP::DOtherCommonwealth;
    case QLocale::Tunisia: return KP::DTunisian;
    case QLocale::Turkey: return KP::DTurkish;
    case QLocale::Turkmenistan: return KP::DCIS;
    case QLocale::TurksAndCaicosIslands: return KP::DOtherCommonwealth;
    case QLocale::TuvaluTerritory: return KP::DOceanaian;
    case QLocale::Uganda: return KP::DOtherCommonwealth;
    case QLocale::Ukraine: return KP::DUkrainian;
    case QLocale::UnitedArabEmirates: return KP::DArabicAsian;
    case QLocale::UnitedKingdom: return KP::DBritish;
    case QLocale::UnitedStates: return KP::DAmerican;
    case QLocale::UnitedStatesOutlyingIslands: return KP::DAmericanAssociates;
    case QLocale::UnitedStatesVirginIslands: return KP::DAmericanAssociates;
    case QLocale::Uruguay: return KP::DOtherLatinAmerican;
    case QLocale::Uzbekistan: return KP::DCIS;
    case QLocale::Vanuatu: return KP::DOceanaian;
    case QLocale::VaticanCity: return KP::DOtherEuropean;
    case QLocale::Venezuela: return KP::DVenezuelan;
    case QLocale::Vietnam: return KP::DIndochinese;
    case QLocale::WallisAndFutuna: return KP::DOtherFrancophone;
    case QLocale::WesternSahara: return KP::DOtherLatino;
    case QLocale::World: return KP::DUnknownNation;
    case QLocale::Yemen: return KP::DArabicAsian;
    case QLocale::Zambia: return KP::DOtherCommonwealth;
    case QLocale::Zimbabwe: return KP::DOtherCommonwealth;
    default: return KP::DUnknownNation;
    }
    return KP::DUnknownNation;
}

KP::AllegianceGroup Ship::allegianceGroup(QLocale::Territory ter) {
    return static_cast<KP::AllegianceGroup>(allegianceSubGroup(ter) >> 4);
}
