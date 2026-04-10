/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "kp.h"
#include <cmath>
#include <QFile>
#include <QJsonArray>
#include <QSettings>
#include "resexotic.h"
#include "resord.h"

extern QFile *logFile;
extern std::unique_ptr<QSettings> settings;

int KP::ardRealPriceHKDCents(int units) {
    static constexpr double factor = 65536.0;
    double basePriceHKD = units * ardCouponUnitHKD;
    double discounted = basePriceHKD
                        / (std::log(factor * basePriceHKD + 1)
                           / std::log(factor + 1));
    return qRound(discounted * 100.0);
}

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

QByteArray KP::clientARDPurchaseAuth(quint64 orderId, bool authorized) {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::ARDPurchaseAuth;
    result["orderid"] = QString::number(orderId);
    result["authorized"] = authorized;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientAdminTestShipRemove() {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::Admingenerateships;
    result["remove"] = true;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientBuy(int equipDef) {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::Develop;
    result["equipid"] = equipDef;
    result["industrial"] = true;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientBuyFromStore(int equipDef) {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::BuyFromStore;
    result["equipid"] = equipDef;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientBuyMedal(int amount) {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::BuyMedal;
    result["amount"] = amount;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientBuyOrdinaryResources(const QString &attr, int coupons) {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::BuyOrdinaryResources;
    result["attr"] = attr;
    result["coupons"] = coupons;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientChooseNode(int mapId, int chosenNodeId) {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::ChooseNode;
    result["mapid"] = mapId;
    result["chosennode"] = chosenNodeId;
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

QByteArray KP::clientDemandMapInfoUser() {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::DemandMapInfoUser;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientDemandDecorate(const QList<QUuid> &candidates) {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::DecorateShip;
    QJsonArray candidatesList;
    for(auto candidate: candidates) {
        candidatesList.append(QJsonValue(candidate.toString()));
    }
    result["shipids"] = candidatesList;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientDemandModernize(
    const QList<QUuid> &candidates, bool isEquip) {
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

QByteArray KP::clientDemandRankInfo(int rowsPerPage,
                                    std::optional<int> pageNum) {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::DemandRankInfo;
    result["rpp"] = rowsPerPage;
    if(pageNum.has_value()) {
        result["page"] = pageNum.value();
    }
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientDemandRepair(const QUuid &uuid, int slotnum,
                                  bool stop, bool forced) {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::Repair;
    result["shipuuid"] = uuid.toString();
    result["slotnum"] = slotnum;
    result["stop"] = stop;
    result["forced"] = forced;
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

QByteArray KP::clientConvertSkillPoints(int srcEquipId, int dstEquipId,
                                        int64 amount) {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::ConvertSkillPoints;
    result["srcequipid"] = srcEquipId;
    result["dstequipid"] = dstEquipId;
    result["amount"] = (qint64)amount;
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

QByteArray KP::clientDockRefresh() {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::Refresh;
    result["view"] = GameState::RepairView;
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

QByteArray KP::clientRequestVisibleBonus(const QUuid &shipUuid,
                                          const QList<QUuid> &equips) {
    QJsonObject result;
    result["type"]    = DgramType::Request;
    result["command"] = CommandType::RequestVisibleBonus;
    result["uuid"]    = shipUuid.toString();
    QJsonArray equipArray;
    for(const QUuid &u : equips)
        equipArray.append(u.toString());
    result["equip"] = equipArray;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientHello() {
    QJsonObject result;
    result["type"] = DgramType::Auth;
    result["command"] = CommandType::CHello;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientHomePort(AllegianceGroup nation) {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::SelectHomePort;
    result["nation"] = nation;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientInitARDPurchase(int units) {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::InitARDPurchase;
    result["units"] = units;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientMigrate(const QJsonObject &input) {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::Migrate;
    result["content"] = input;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientQueryNextNode(int mapId, int prevNode, bool retreat) {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::ProgressMap;
    result["mapid"] = mapId; // absolute id
    result["prevnode"] = prevNode;
    result["retreat"] = retreat;
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

QByteArray KP::clientCancelExpedition(int mapUnionId, int receiveFleetIndex) {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::CancelExpedition;
    result["mapid"] = mapUnionId;
    result["receivefleetindex"] = receiveFleetIndex;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientQueryExpeditionStatus() {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::QueryExpeditionStatus;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientSetExpeditionSettings(int mapUnionId, double autoResupplyThreshold,
                                           bool autoRestart) {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::SetExpeditionSettings;
    result["mapid"] = mapUnionId;
    result["autoResupplyThreshold"] = autoResupplyThreshold;
    result["autoRestart"] = autoRestart;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientStartExpedition(int mapUnionId, int fleetIndex,
                                     const QMap<int, QByteArray> &battlePlans,
                                     double autoResupplyThreshold) {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::StartExpedition;
    result["mapid"] = mapUnionId;
    result["fleetindex"] = fleetIndex;
    result["autoResupplyThreshold"] = autoResupplyThreshold;
    QJsonObject plansObj;
    for(auto it = battlePlans.begin(); it != battlePlans.end(); ++it) {
        plansObj[QString::number(it.key())] = QString(it.value().toBase64());
    }
    result["battlePlans"] = plansObj;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientUpdateExpeditionPlan(int mapUnionId,
                                          const QMap<int, QByteArray> &battlePlans) {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::UpdateExpeditionPlan;
    result["mapid"] = mapUnionId;
    QJsonObject plansObj;
    for(auto it = battlePlans.begin(); it != battlePlans.end(); ++it) {
        plansObj[QString::number(it.key())] = QString(it.value().toBase64());
    }
    result["battlePlans"] = plansObj;
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

QByteArray KP::clientSupplyShip(const QJsonArray &ships) {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::SupplyShip;
    result["ships"] = ships;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientTestMessages(int index) {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::MessageTest;
    result["id"] = index;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverARDPurchaseClawback(int unitsDeducted) {
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = MsgType::ARDPurchaseClawback;
    result["units"] = unitsDeducted;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverARDPurchaseFailed(PurchaseFailReason reason) {
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = MsgType::ARDPurchaseFailed;
    result["reason"] = static_cast<int>(reason);
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverARDPurchasePending(quint64 orderId) {
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = MsgType::ARDPurchasePending;
    result["orderid"] = QString::number(orderId);
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverARDPurchaseSuccess(int unitsAdded) {
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = MsgType::ARDPurchaseSuccess;
    result["units"] = unitsAdded;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverAskForHomePort()
{
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = MsgType::AskForHomePort;
    QJsonArray array;
    QList<KP::AllegianceGroup> availableHomePorts
        = {KP::Japanese};
    for(auto homeport: availableHomePorts) {
        array.append(homeport);
    }
    result["choices"] = array;
    /*
    QList<KP::ShipNationalityGroup> availableHomePorts
        = {KP::Japanese, KP::German, KP::Italian,
           KP::American, KP::British, KP::French, KP::Soviet,
           KP::Commonwealth};
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
    result["msgtype"] =
        construct ? MsgType::ConstructStart : MsgType::DevelopStart;
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

QByteArray KP::serverEquipmentDamaged(int equipDef, int deduction, int sameTypeCount) {
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = MsgType::EquipmentDamaged;
    result["equipdef"] = equipDef;
    result["deduction"] = deduction;
    result["sameTypeCount"] = sameTypeCount;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverPlaneLossSkillDeduction(int equipDef, int totalLosses, int totalDeduction, int deductions, int deductionPer100) {
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = MsgType::PlaneLossSkillDeduction;
    result["equipdef"] = equipDef;
    result["totallosses"] = totalLosses;
    result["totaldeduction"] = totalDeduction;
    result["deductions"] = deductions;
    result["deductionper100"] = deductionPer100;
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

QByteArray KP::serverFleetFailure(FleetFailType type, int fleetIndex) {
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = MsgType::FleetFail;
    result["reason"] = type;
    if(fleetIndex >= 0)
        result["fleetindex"] = fleetIndex;
    return QCborValue::fromJsonValue(result).toCbor();
}

/* content must contain a "ships" array; each element has "uuid",
 * "bonuses" (first type, per slot), and "bonuses2" (second type, LuaMap). */
QByteArray KP::serverVisibleBonusInfo(const QJsonObject &content) {
    QJsonObject result;
    result["type"]     = DgramType::Info;
    result["infotype"] = InfoType::VisibleBonusInfo;
    result["ships"]    = content["ships"];
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

QByteArray KP::serverMapInfo(const QJsonArray &input,
                             QDateTime timeUtc, bool cacheHit) {
    QJsonObject result;
    result["type"] = DgramType::Info;
    result["infotype"] = InfoType::MapInfo;
    result["timestamp"] = timeUtc.toString();
    if(!cacheHit) {
        result["content"] = input;
    }
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverMapInfoUser(const QJsonObject &input,
                                  AllegianceGroup homePort) {
    QJsonObject result;
    result["type"] = DgramType::Info;
    result["infotype"] = InfoType::MapInfoUser;
    result["homeport"] = static_cast<int>(homePort);
    result["content"] = input;
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

QByteArray KP::serverDisasterLOSInfo(double requiredLOS, double fleetLOS,
                                     double chanceToAvoid, double fuelFrac,
                                     double ammoFrac, bool deductionOccurred) {
    QJsonObject result;
    result["type"] = DgramType::Info;
    result["infotype"] = InfoType::DisasterLOSInfo;
    result["requiredLOS"] = requiredLOS;
    result["fleetLOS"] = fleetLOS;
    result["chanceToAvoid"] = chanceToAvoid;
    result["fuelFrac"] = fuelFrac;
    result["ammoFrac"] = ammoFrac;
    result["deductionOccurred"] = deductionOccurred;
    return QCborValue::fromJsonValue(result).toCbor();
}

 QByteArray KP::serverTransportFreightInfo(int currentFreight, int capacity,
                                      int added) {
    QJsonObject result;
    result["type"] = DgramType::Info;
    result["infotype"] = InfoType::TransportFreightInfo;
    result["currentFreight"] = currentFreight;
    result["capacity"] = capacity;
    result["added"] = added;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverPlaneReplenishResult(GameError error, const ResOrd &cost) {
    QJsonObject result;
    result["type"] = DgramType::Info;
    result["infotype"] = InfoType::PlaneReplenishResult;
    result["error"] = static_cast<int>(error);
    result["cost_o"] = cost.o;
    result["cost_e"] = cost.e;
    result["cost_s"] = cost.s;
    result["cost_r"] = cost.r;
    result["cost_a"] = cost.a;
    result["cost_w"] = cost.w;
    result["cost_c"] = cost.c;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverMapStart(int mapId, int startNode) {
    QJsonObject result;
    result["type"] = DgramType::Info;
    result["infotype"] = InfoType::MapStart;
    result["mapid"] = mapId; // absolute id
    result["start"] = startNode;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverExpeditionProgressUpdate(int mapUnionId, int nodeIndex,
                                              const QJsonObject &battleResult) {
    QJsonObject result;
    result["type"] = DgramType::Info;
    result["infotype"] = InfoType::ExpeditionProgressUpdate;
    result["mapid"] = mapUnionId;
    result["nodeindex"] = nodeIndex;
    result["battleResult"] = battleResult;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverExpeditionStartResult(int mapUnionId, bool accepted,
                                           const QString &errorReason) {
    QJsonObject result;
    result["type"] = DgramType::Info;
    result["infotype"] = InfoType::ExpeditionStartResult;
    result["mapid"] = mapUnionId;
    result["accepted"] = accepted;
    if(!errorReason.isEmpty())
        result["errorReason"] = errorReason;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverExpeditionStatus(const QJsonArray &expeditions) {
    QJsonObject result;
    result["type"] = DgramType::Info;
    result["infotype"] = InfoType::ExpeditionStatus;
    result["expeditions"] = expeditions;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverExpeditionStopped(int mapUnionId, int stopReason) {
    QJsonObject result;
    result["type"] = DgramType::Info;
    result["infotype"] = InfoType::ExpeditionStopped;
    result["mapid"] = mapUnionId;
    result["stopReason"] = stopReason;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverMedalPurchased(int amount) {
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = MsgType::MedalPurchased;
    result["amount"] = amount;
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

QByteArray KP::serverNewmodelShip(QUuid serial, int shipDid, int initialHP) {
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = MsgType::ShipRemodeled;
    result["serial"] = serial.toString();
    result["shipdef"] = shipDid;
    result["hp"] = initialHP;
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

QByteArray KP::serverRankInfo(const QJsonArray &content,
                              int totalUsers,
                              std::optional<double> yourIP) {
    QJsonObject result;
    result["type"] = DgramType::Info;
    result["infotype"] = InfoType::RankInfo;
    result["content"] = content;
    result["total"] = totalUsers;
    if(yourIP.has_value()) {
        result["yourip"] = yourIP.value();
    }
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverResourceUpdate(ResOrd ordinary, ResExotic exotic) {
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
    result["ardcoupon"] = exotic.ard;
    result["medal"]     = exotic.medal;
    result["sanity"]    = exotic.sanity;
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

QByteArray KP::serverShipDecorated(
    const QList<std::tuple<QUuid, int>> &ships) {
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = MsgType::ShipDecorated;
    QJsonArray shipList;
    for(auto shipTuple: ships) {
        QJsonObject item;
        item["shipid"] = QJsonValue(std::get<0>(shipTuple).toString());
        item["newexpcap"] = std::get<1>(shipTuple);
        shipList.append(item);
    }
    result["shipdata"] = shipList;
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

QByteArray KP::serverSkillPointConvertResult(int srcEquipId, int dstEquipId,
                                             bool success, int64 newSrcSP,
                                             int64 newDstSP) {
    QJsonObject result;
    result["type"] = DgramType::Info;
    result["infotype"] = InfoType::SkillPointConvertResult;
    result["srcequipid"] = srcEquipId;
    result["dstequipid"] = dstEquipId;
    result["success"] = success;
    result["newSrcSP"] = (qint64)newSrcSP;
    result["newDstSP"] = (qint64)newDstSP;
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

