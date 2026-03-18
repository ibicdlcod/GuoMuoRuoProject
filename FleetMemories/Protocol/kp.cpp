/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "kp.h"
#include <QFile>
#include <QJsonArray>
#include <QSettings>
#include "resord.h"

extern QFile *logFile;
extern std::unique_ptr<QSettings> settings;

void KP::initLog(bool server) {
    QString logFileName;
    if(server) {
        logFileName = settings->value("server/logfile",
                                      "ServerLog.log").toString();
    }
    else {
        logFileName = settings->value("client/logfile",
                                      "ClientLog.log").toString();
    }
    logFile = new QFile(logFileName);
    if(Q_UNLIKELY(!logFile)
        || !logFile->open(QIODevice::WriteOnly | QIODevice::Append)) {
        qFatal("Log file cannot be opened");
    }
}

#if defined (Q_OS_WIN)
void KP::winConsoleCheck() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) {
        throw GetLastError();
    }
    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) {
        //% "This program must be run in the terminal."
        qFatal(qtTrId("terminial-required").toUtf8());
        throw GetLastError();
    }
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (!SetConsoleMode(hOut, dwMode)) {
        throw GetLastError();
    }
}
#endif

QByteArray KP::accessDenied() {
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = MsgType::AccessDenied;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::catbomb() {
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = MsgType::AllowClientFinish;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientAddEquip(int equipid) {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::Adminaddequip;
    result["equipid"] = equipid;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientAdminTestEquip() {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::Admingenerateequips;
    result["remove"] = false;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientAdminTestEquipRemove() {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::Admingenerateequips;
    result["remove"] = true;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientAdminTestShip() {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::Admingenerateships;
    result["remove"] = false;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientAdminTestShipRemove() {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::Admingenerateships;
    result["remove"] = true;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientConstruct(int shipDef,
                               const QList<QUuid> &defaultEquips,
                               QUuid &shipToRemodel,
                               int factoryID) {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::Construct;
    result["shipdef"] = shipDef;
    QJsonArray equipArray;
    for(auto equip: defaultEquips) {
        equipArray.append(QJsonValue(equip.toString()));
    }
    result["defaultequip"] = equipArray;
    result["shiptoremodel"] = shipToRemodel.toString();
    result["factory"] = factoryID;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientDemandDestructEquip(const QList<QUuid> &trash) {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::DestructEquip;
    QJsonArray trashList;
    for(auto trashItem: trash) {
        trashList.append(QJsonValue(trashItem.toString()));
    }
    result["equipids"] = trashList;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientDemandEquipInfo(QDateTime timeUtc) {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::DemandEquipInfo;
    result["timestamp"] = timeUtc.toString();
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientDemandEquipInfoUser() {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::DemandEquipInfoUser;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientDemandMapInfo(QDateTime timeUtc) {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::DemandMapInfo;
    result["timestamp"] = timeUtc.toString();
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientDemandModernize(const QList<QUuid> &candidates, bool isEquip) {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::Modernize;
    QJsonArray candidatesList;
    for(auto candidate: candidates) {
        candidatesList.append(QJsonValue(candidate.toString()));
    }
    result["equipids"] = candidatesList;
    result["isequip"] = isEquip;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientDemandResourceUpdate() {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::DemandResourceUpdate;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientDemandShipInfo(QDateTime timeUtc) {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::DemandShipInfo;
    result["timestamp"] = timeUtc.toString();
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientDemandShipInfoUser() {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::DemandShipInfoUser;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientDemandSkillPoints(int equipId) {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::DemandSkillPoints;
    result["equipid"] = equipId;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientDemandTech(int local) {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::DemandTech;
    result["local"] = local;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientDevelop(int equipid, bool convert, int factoryID) {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::Develop;
    result["equipid"] = equipid;
    result["convert"] = convert;
    result["factory"] = factoryID;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientDoBattleNode(const QJsonObject &contents) {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::EnterBattleNode;
    result["content"] = contents;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientFactoryRefresh() {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::Refresh;
    result["view"] = GameState::Factory;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientFetch(int factoryID, bool forced) {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::Fetch;
    result["factory"] = factoryID;
    result["forced"] = forced;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientFleetData(const QJsonArray &input) {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::FleetData;
    result["content"] = input;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientHello() {
    QJsonObject result;
    result["type"] = DgramType::Auth;
    result["command"] = CommandType::CHello;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientHomePort(ShipNationalityGroup nation) {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::SelectHomePort;
    result["nation"] = nation;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientMigrate(const QJsonObject &input) {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::Migrate;
    result["content"] = input;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientQueryNextNode(int mapId, int prevNode) {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::ProgressMap;
    result["mapid"] = mapId; // absolute id
    result["prevnode"] = prevNode;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientSortie(int mapId, int fleetIndex, bool expedition) {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::RequestSortie;
    result["mapid"] = mapId;
    result["fleetindex"] = fleetIndex;
    result["expedition"] = expedition;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientStateChange(GameState state) {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::ChangeState;
    result["state"] = state;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientSteamAuth(uint8 rgubTicket [], uint32 cubTicket) {
    QJsonObject result;
    result["type"] = DgramType::Auth;
    result["command"] = CommandType::SteamAuth;
    QJsonArray rgubArray = QJsonArray();
    for(unsigned int i = 0; i < cubTicket; ++i) {
        rgubArray.append(rgubTicket[i]);
    }
    result["rgubTicket"] = rgubArray;
    result["cubTicket"] = (qint64)cubTicket;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientSteamLogout() {
    QJsonObject result;
    result["type"] = DgramType::Auth;
    result["command"] = CommandType::SteamLogout;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientTestMessages(int index) {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::MessageTest;
    result["id"] = index;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverAskForHomePort()
{
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = MsgType::AskForHomePort;
    QJsonArray array;
    QList<KP::ShipNationalityGroup> availableHomePorts
        = {KP::Japanese};
    for(auto homeport: availableHomePorts) {
        array.append(homeport);
    }
    result["choices"] = array;
    /*
    QList<KP::ShipNationalityGroup> availableHomePorts
        = {KP::Japanese, KP::German, KP::Italian,
           KP::American, KP::British, KP::French, KP::Soviet,
           KP::Oceanian};
    */
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverBattleEnd() {
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = MsgType::BattleProcess;
    result["end"] = true;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverBattleError(GameError error) {
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = MsgType::BattleError;
    result["reason"] = error;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverBattleProcess(const QJsonObject &contents) {
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = MsgType::BattleProcess;
    result["content"] = contents;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverBlueprintAdded(int shipDef) {
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = MsgType::ShipBPAdded;
    result["shipdef"] = shipDef;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverBlueprintRetired(int shipDef) {
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = MsgType::ShipBPRetired;
    result["shipdef"] = shipDef;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverDevelopFailed(GameError error) {
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = MsgType::DevelopFailed;
    result["reason"] = error;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverDevelopStart(bool construct) {
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = construct ? MsgType::ConstructStart : MsgType::DevelopStart;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverDisableShip(QUuid serial) {
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = MsgType::DisableShip;
    result["serial"] = serial.toString();
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverEquipLackFather(GameError error, int father) {
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = MsgType::DevelopFailed;
    result["reason"] = error;
    result["father"] = father;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverEquipLackMother(GameError error, int mother, int64 sp) {
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = MsgType::DevelopFailed;
    result["reason"] = error;
    result["mother"] = mother;
    result["skillpoint"] = (qint64)sp;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverEquipRetired(const QList<QUuid> &trash) {
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = MsgType::EquipRetired;
    QJsonArray trashList;
    for(auto trashItem: trash) {
        trashList.append(QJsonValue(trashItem.toString()));
    }
    result["equipids"] = trashList;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverEquipImproved(
    const QList<std::tuple<QUuid, int>> &equips) {
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = MsgType::EquipImproved;
    QJsonArray equipList;
    for(auto equipTuple: equips) {
        QJsonObject item;
        item["equipid"] = QJsonValue(std::get<0>(equipTuple).toString());
        item["newstar"] = std::get<1>(equipTuple);
        equipList.append(item);
    }
    result["equipdata"] = equipList;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverEquipInfo(const QJsonArray &input, bool user,
                               QDateTime timeUtc, bool cacheHit) {
    QJsonObject result;
    result["type"] = DgramType::Info;
    if(user)
        result["infotype"] = InfoType::EquipInfoUser;
    else {
        result["infotype"] = InfoType::EquipInfo;
        result["timestamp"] = timeUtc.toString();
    }
    if(!cacheHit) {
        result["content"] = input;
    }
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverFairyBusy(int jobID) {
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = MsgType::FairyBusy;
    result["job"] = jobID;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverFleetFailure(FleetFailType type) {
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = MsgType::FleetFail;
    result["reason"] = type;
    return QCborValue::fromJsonValue(result).toCbor();
}

/* actually, both local and global use this function */
QByteArray KP::serverGlobalTech(double tech, int jobId) {
    QJsonObject result;
    result["type"] = DgramType::Info;
    result["infotype"] = (jobId == 0)
                             ? InfoType::GlobalTechInfo
                             : InfoType::LocalTechInfo;
    result["value"] = tech;
    result["jobid"] = jobId;
    return QCborValue::fromJsonValue(result).toCbor();
}

/* actually, both local and global use this function */
QByteArray KP::serverGlobalTech(const QList<TechEntry> &content, bool global) {
    QJsonObject result;
    result["type"] = DgramType::Info;
    result["infotype"] = global
                             ? InfoType::GlobalTechInfo
                             : InfoType::LocalTechInfo;
    QJsonArray contentJSON;
    for(auto &contentPart: content) {
        QJsonObject part;
        part["serial"] = std::get<0>(contentPart).toString();
        part["def"] = std::get<1>(contentPart);
        part["weight"] = std::get<2>(contentPart);
        contentJSON.append(part);
    }
    result["content"] = contentJSON;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverHello() {
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = MsgType::Hello;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverLackPrivate() {
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = MsgType::LackPrivate;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverLogFail(KP::AuthFailType reason) {
    QJsonObject result;
    result["type"] = DgramType::Auth;
    result["mode"] = AuthMode::NewLogin;
    result["success"] = false;
    result["reason"] = reason;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverLogSuccess(bool newUser) {
    QJsonObject result;
    result["type"] = DgramType::Auth;
    result["mode"] = AuthMode::NewLogin;
    result["success"] = true;
    result["newuser"] = newUser;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverLogout(KP::LogoutType reason) {
    QJsonObject result;
    result["type"] = DgramType::Auth;
    result["mode"] = AuthMode::Logout;
    result["reason"] = reason;
    result["success"] = (reason != KP::LogoutFailure);
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverMapInfo(const QJsonArray &input, bool user,
                             QDateTime timeUtc, bool cacheHit) {
    QJsonObject result;
    result["type"] = DgramType::Info;
    if(user)
        result["infotype"] = InfoType::MapInfoUser;
    else {
        result["infotype"] = InfoType::MapInfo;
        result["timestamp"] = timeUtc.toString();
    }
    if(!cacheHit) {
        result["content"] = input;
    }
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverMapNotOpen(int mapId) {
    QJsonObject result;
    result["type"] = DgramType::Info;
    result["infotype"] = InfoType::MapInfo;
    result["bad"] = true;
    result["mapid"] = mapId;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverMapProgress(int mapId, int nextNode) {
    QJsonObject result;
    result["type"] = DgramType::Info;
    result["infotype"] = InfoType::MapProgress;
    result["mapid"] = mapId; // relative id
    result["next"] = nextNode;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverMapStart(int mapId, int startNode) {
    QJsonObject result;
    result["type"] = DgramType::Info;
    result["infotype"] = InfoType::MapStart;
    result["mapid"] = mapId; // relative id
    result["start"] = startNode;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverNewEquip(QUuid serial, int equipDid) {
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = MsgType::NewEquip;
    result["serial"] = serial.toString();
    result["equipdef"] = equipDid;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverNewShip(QUuid serial, int shipDid, int initialHP) {
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = MsgType::NewShip;
    result["serial"] = serial.toString();
    result["shipdef"] = shipDid;
    result["hp"] = initialHP;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverNewmodelShip(QUuid serial, int shipDid, int initialHP) {
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = MsgType::ShipRemodeled;
    result["serial"] = serial.toString();
    result["shipdef"] = shipDid;
    result["hp"] = initialHP;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverParseError(MsgType pe, const QString &uname,
                                const QString &content) {
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = pe;
    result["username"] = uname;
    result["content"] = content;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverPenguin() {
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = MsgType::Penguin;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverResourceUpdate(ResOrd ordinary) {
    QJsonObject result;
    result["type"] = DgramType::Info;
    result["infotype"] = InfoType::ResourceInfo;
    result["oil"] = ordinary.o;
    result["explo"] = ordinary.e;
    result["steel"] = ordinary.s;
    result["rubber"] = ordinary.r;
    result["al"] = ordinary.a;
    result["w"] = ordinary.w;
    result["cr"] = ordinary.c;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverShipBPInfo(const QJsonObject &input) {
    QJsonObject result;
    result["type"] = DgramType::Info;
    result["infotype"] = InfoType::ShipInfoUserBP;
    result["content"] = input;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverShipInfo(const QJsonArray &input, bool user,
                              QDateTime timeUtc, bool cacheHit) {
    QJsonObject result;
    result["type"] = DgramType::Info;
    if(user)
        result["infotype"] = InfoType::ShipInfoUser;
    else {
        result["infotype"] = InfoType::ShipInfo;
        result["timestamp"] = timeUtc.toString();
    }
    if(!cacheHit) {
        result["content"] = input;
    }
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverShipModernized(
    const QList<std::tuple<QUuid, int>> &ships) {
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = MsgType::ShipModernized;
    QJsonArray shipList;
    for(auto shipTuple: ships) {
        QJsonObject item;
        item["shipid"] = QJsonValue(std::get<0>(shipTuple).toString());
        item["newstar"] = std::get<1>(shipTuple);
        shipList.append(item);
    }
    result["shipdata"] = shipList;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverSkillPoints(int equipId,
                                 int64 currentSP, int64 standardSP) {
    QJsonObject result;
    result["type"] = DgramType::Info;
    result["infotype"] = InfoType::SkillPointInfo;
    result["equipid"] = equipId;
    result["actualSP"] = (qint64)currentSP;
    result["desiredSP"] = (qint64)standardSP;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverSuccess() {
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = MsgType::Success;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverTestMessages(int index) {
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = MsgType::MessageTestServer;
    result["id"] = index;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverVerifyComplete() {
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = MsgType::VerifyComplete;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::weighAnchor() {
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = MsgType::AllowClientStart;
    return QCborValue::fromJsonValue(result).toCbor();
}

KP::ShipNationalitySubGroup KP::nationalityTable(QLocale::Territory ter) {
    /* The World War II involved not only the great powers but the people of subjugated colonies and practically subjugated countries.
     * The great contribution or struggle of them subsequently made the colonial system became generally unviable and mostly replaced
     * by international organizations based on lingua franca. To offer respect to them, all countries/territories (that Qt recognizes)
     * is included in this function, even if they are landlocked, or not independent during WWII, or have no navy to speak of even today.
     *
     * If a great power's ship is primarily deployed to colonies during WWII, its nationality will count as that of said colony here.
     * An example is De Ruyter, who is regarded as Dutch East Indian and dedicated to Indonesian people in this game, rather than Dutch.
     *
     * The below "nationality sub-groups" is primarily determined by status during WWII rather than modern status. Exceptions are made,
     * such as South Korea having a significant navy in modern times, so Korea's space is not lumped with Japan's, which also have a
     * large navy, despite Korea is Japan's colony during WWII. In paticular, the first hex digit "nationality group" is even more
     * heavily based on WWII, regardless of how little these countries are connected in modern times.
     *
     * It's useless to pretend that no political points are made in this function; disagreers should fork this project instead of
     * complaining. */

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
    case QLocale::Austria: return KP::DGerman; /* the primary focus is WWII */
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
    case QLocale::Bolivia: return KP::DOtherLatino;
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
    case QLocale::CostaRica: return KP::DOtherLatino;
    case QLocale::Croatia: return KP::DYugoslavian;
    case QLocale::Cuba: return KP::DCuban;
    case QLocale::Curacao: return KP::DDutchSpeakingAmericas;
    case QLocale::Cyprus: return KP::DGreekOrCypriot;
    case QLocale::Czechia: return KP::DOtherEuropean;
    case QLocale::Denmark: return KP::DDanish;
    case QLocale::DiegoGarcia: return KP::DOtherCommonwealth;
    case QLocale::Djibouti: return KP::DOtherFrancophone;
    case QLocale::Dominica: return KP::DOtherLatino;
    case QLocale::DominicanRepublic: return KP::DOtherLatino;
    case QLocale::Ecuador: return KP::DColumbianOrEcuadoran;
    case QLocale::Egypt: return KP::DEgyptian;
    case QLocale::ElSalvador: return KP::DOtherLatino;
    case QLocale::EquatorialGuinea: return KP::DOtherLatino;
    case QLocale::Eritrea: return KP::DEastAfrican;
    case QLocale::Estonia: return KP::DBaltic;
    case QLocale::Eswatini: return KP::DOtherCommonwealth;
    case QLocale::Ethiopia: return KP::DEastAfrican;
    case QLocale::EuropeanUnion: return KP::DOtherEuropean;
    case QLocale::Europe: return KP::DOtherEuropean;
    case QLocale::FalklandIslands: return KP::DArgentinian; // just use QLocale::UnitedKingdom->DBritish for actual British ships
    case QLocale::FaroeIslands: return KP::DDanishKingdom;
    case QLocale::Fiji: return KP::DOceanaian;
    case QLocale::Finland: return KP::DFinnish;
    case QLocale::France: return KP::DFrench;
    case QLocale::FrenchGuiana: return KP::DFrench; /* overseas departments (and not other dependencies of France) use DFrench */
    case QLocale::FrenchPolynesia: return KP::DOtherFrancophone;
    case QLocale::FrenchSouthernTerritories: return KP::DOtherFrancophone;
    case QLocale::Gabon: return KP::DOtherFrancophone;
    case QLocale::Gambia: return KP::DOtherCommonwealth;
    case QLocale::Georgia: return KP::DCIS;
    case QLocale::Germany: return KP::DGerman;
    case QLocale::Ghana: return DOtherCommonwealth;
    case QLocale::Gibraltar: return DOtherEuropean; // just use QLocale::UnitedKingdom->DBritish for actual British ships
    case QLocale::Greece: return DGreekOrCypriot;
    case QLocale::Greenland: return DDanishKingdom;
    case QLocale::Grenada: return DOtherCommonwealth;
    case QLocale::Guadeloupe: return DFrench;
    case QLocale::Guam: return DAmericanAssociates;
    case QLocale::Guatemala: return DOtherLatino;
    case QLocale::Guernsey: return DOtherCommonwealth;
    case QLocale::Guinea: return DOtherFrancophone;
    case QLocale::GuineaBissau: return DOtherLatino;
    case QLocale::Guyana: return DOtherCommonwealth;
    case QLocale::Haiti: return DOtherLatino;
    case QLocale::HeardAndMcDonaldIslands: return DOceanaian;
    case QLocale::Honduras: return DOtherLatino;
    case QLocale::HongKong: return DChineseOther; // just use QLocale::China->DChineseModern after 1997
    case QLocale::Hungary: return DOtherEuropean;
    case QLocale::Iceland: return DIcelandic;
    case QLocale::India: return DIndian;
    case QLocale::Indonesia: return DIndonesian;
    case QLocale::Iran: return DIranian;
    case QLocale::Iraq: return DArabicAsian;
    case QLocale::Ireland: return DIrish;
    case QLocale::IsleOfMan: return DOtherCommonwealth;
    case QLocale::Israel: return DIsraeli;
    case QLocale::Italy: return DItalian;
    case QLocale::IvoryCoast: return DOtherFrancophone;
    case QLocale::Jamaica: return DOtherCommonwealth;
    case QLocale::Japan: return DJapanese;
    case QLocale::Jersey: return DOtherCommonwealth;
    case QLocale::Jordan: return DArabicAsian;
    case QLocale::Kazakhstan: return DCIS;
    case QLocale::Kenya: return DOtherCommonwealth;
    case QLocale::Kiribati: return DOceanaian;
    // Kosovo: see "S" section below
    case QLocale::Kuwait: return DArabicAsian;
    case QLocale::Kyrgyzstan: return DCIS;
    case QLocale::Laos: return DIndochinese;
    case QLocale::LatinAmerica: return DOtherLatino;
    case QLocale::Latvia: return DBaltic;
    case QLocale::Lebanon: return DArabicAsian;
    case QLocale::Lesotho: return DOtherCommonwealth;
    case QLocale::Liberia: return DAmericanAssociates;
    case QLocale::Libya: return DLibyan;
    case QLocale::Liechtenstein: return DOtherEuropean;
    case QLocale::Lithuania: return DBaltic;
    case QLocale::Luxembourg: return DBeneluxOther;
    case QLocale::Macao: return DChineseOther; // just use QLocale::China->DChineseModern after 1999
    case QLocale::Macedonia: return DYugoslavian;
    case QLocale::Madagascar: return DOtherFrancophone;
    case QLocale::Malawi: return DOtherCommonwealth;
    case QLocale::Malaysia: return DMalaysianOrBruneian;
    case QLocale::Maldives: return DOtherCommonwealth;
    case QLocale::Mali: return DOtherFrancophone;
    case QLocale::Malta: return DOtherCommonwealth;
    case QLocale::MarshallIslands: return DAmericanAssociates; //(COFA)
    case QLocale::Martinique: return DFrench;
    case QLocale::Mauritania: return DMauritanian;
    case QLocale::Mauritius: return DOtherCommonwealth;
    case QLocale::Mayotte: return DFrench;
    case QLocale::Mexico: return DMexican;
    case QLocale::Micronesia: return DAmericanAssociates; //(COFA)
    case QLocale::Moldova: return DCIS;
    case QLocale::Monaco: return DOtherFrancophone;
    case QLocale::Mongolia: return DMongolian;
    case QLocale::Montenegro: return DYugoslavian;
    case QLocale::Montserrat: return DOtherCommonwealth;
    case QLocale::Morocco: return DMoroccoan;
    case QLocale::Mozambique: return DOtherLatino;
    case QLocale::Myanmar: return DOtherAsian;
    case QLocale::Namibia: return DSouthAfricanOrNamibian;
    case QLocale::NauruTerritory: return DOceanaian;
    case QLocale::Nepal: return DOtherAsian;
    case QLocale::Netherlands: return DDutch;
    case QLocale::NewCaledonia: return DOtherFrancophone;
    case QLocale::NewZealand: return DNewZealander;
    case QLocale::Nicaragua: return DOtherLatino;
    case QLocale::Niger: return DOtherFrancophone;
    case QLocale::Nigeria: return DOtherCommonwealth;
    case QLocale::Niue: return DOtherCommonwealth;
    case QLocale::NorfolkIsland: return DAustralian;
    case QLocale::NorthernMarianaIslands: return DAmericanAssociates;
    case QLocale::NorthKorea: return DNorthKorean;
    case QLocale::Norway: return DNorwegian;
    case QLocale::Oman: return DArabicAsian;
    case QLocale::OutlyingOceania: return DOceanaian;
    case QLocale::Pakistan: return DPakistani;
    case QLocale::Palau: return DAmericanAssociates; //(COFA)
    case QLocale::PalestinianTerritories: return DArabicAsian;
    case QLocale::Panama: return DOtherLatino;
    case QLocale::PapuaNewGuinea: return DOceanaian;
    case QLocale::Paraguay: return DOtherLatino;
    case QLocale::Peru: return DOtherLatino;
    case QLocale::Philippines: return DFilipino;
    case QLocale::Pitcairn: return DOtherCommonwealth;
    case QLocale::Poland: return DPolish;
    case QLocale::Portugal: return DPortuguese;
    case QLocale::PuertoRico: return DAmericanAssociates;
    case QLocale::Qatar: return DArabicAsian;
    case QLocale::Reunion: return DFrench;
    case QLocale::Romania: return DRomanian;
    /* Soviet ships that are named after Baltic states cities also goes here,
     * as they would belong to DBaltic->EasternEuropean otherwise */
    case QLocale::Russia: return DSovietOrRussian;
    case QLocale::Rwanda: return DCentralAfrican;
    case QLocale::SaintBarthelemy: return DOtherFrancophone;
    case QLocale::SaintHelena: return DOtherCommonwealth;
    case QLocale::SaintKittsAndNevis: return DOtherCommonwealth;
    case QLocale::SaintLucia: return DOtherCommonwealth;
    case QLocale::SaintMartin: return DOtherFrancophone;
    case QLocale::SaintPierreAndMiquelon: return DOtherFrancophone;
    case QLocale::SaintVincentAndGrenadines: return DOtherCommonwealth;
    case QLocale::Samoa: return DOceanaian;
    case QLocale::SanMarino: return DItalian; // done for linguistic reasons
    case QLocale::SaoTomeAndPrincipe: return DOtherLatino;
    case QLocale::SaudiArabia: return DArabicAsian;
    case QLocale::Senegal: return DOtherFrancophone;
    case QLocale::Kosovo: [[fallthrough]];
    case QLocale::Serbia: return DYugoslavian;
    case QLocale::Seychelles: return DOtherFrancophone;
    case QLocale::SierraLeone: return DOtherCommonwealth;
    case QLocale::Singapore: return DSingaporean;
    case QLocale::SintMaarten: return DDutchSpeakingAmericas;
    case QLocale::Slovakia: return DOtherEuropean;
    case QLocale::Slovenia: return DYugoslavian;
    case QLocale::SolomonIslands: return DOceanaian;
    case QLocale::Somalia: return DEastAfrican;
    case QLocale::SouthAfrica: return DSouthAfricanOrNamibian;
    case QLocale::SouthGeorgiaAndSouthSandwichIslands: return DOtherCommonwealth;
    case QLocale::SouthKorea: return DSouthKorean;
    case QLocale::SouthSudan: return DOtherCommonwealth;
    case QLocale::Spain: return DSpanish;
    case QLocale::SriLanka: return DOtherCommonwealth;
    case QLocale::Sudan: return DOtherCommonwealth;
    case QLocale::Suriname: return DDutchSpeakingAmericas;
    case QLocale::SvalbardAndJanMayen: return DNorwegian;
    case QLocale::Sweden: return DSwedish;
    case QLocale::Switzerland: return DOtherEuropean;
    case QLocale::Syria: return DArabicAsian;
    case QLocale::Taiwan: return DChineseNationalist;
    case QLocale::Tajikistan: return DCIS;
    case QLocale::Tanzania: return DOtherCommonwealth;
    case QLocale::Thailand: return DThai;
    case QLocale::TimorLeste: return DOtherLatino;
    case QLocale::Togo: return DOtherFrancophone;
    case QLocale::TokelauTerritory: return DOceanaian;
    case QLocale::Tonga: return DOceanaian;
    case QLocale::TrinidadAndTobago: return DOtherLatino;
    case QLocale::TristanDaCunha: return DOtherCommonwealth;
    case QLocale::Tunisia: return DTunisian;
    case QLocale::Turkey: return DTurkish;
    case QLocale::Turkmenistan: return DCIS;
    case QLocale::TurksAndCaicosIslands: return DOtherCommonwealth;
    case QLocale::TuvaluTerritory: return DOceanaian;
    case QLocale::Uganda: return DOtherCommonwealth;
    case QLocale::Ukraine: return DUkrainian;
    case QLocale::UnitedArabEmirates: return DArabicAsian;
    case QLocale::UnitedKingdom: return DBritish;
    case QLocale::UnitedStates: return DAmerican;
    case QLocale::UnitedStatesOutlyingIslands: return DAmericanAssociates;
    case QLocale::UnitedStatesVirginIslands: return DAmericanAssociates;
    case QLocale::Uruguay: return DOtherLatino;
    case QLocale::Uzbekistan: return DCIS;
    case QLocale::Vanuatu: return DOceanaian;
    case QLocale::VaticanCity: return DOtherEuropean;
    case QLocale::Venezuela: return DVenezuelan;
    case QLocale::Vietnam: return DIndochinese;
    case QLocale::WallisAndFutuna: return DOtherFrancophone;
    case QLocale::WesternSahara: return DOtherLatino;
    case QLocale::World: return DUnknownNation;
    case QLocale::Yemen: return DArabicAsian;
    case QLocale::Zambia: return DOtherCommonwealth;
    case QLocale::Zimbabwe: return DOtherCommonwealth;
    default: return DUnknownNation;
    }
    return DUnknownNation;
}

KP::ShipNationalityGroup KP::generalNationalityTable(QLocale::Territory ter) {
    return static_cast<KP::ShipNationalityGroup>(nationalityTable(ter) >> 4);
}
