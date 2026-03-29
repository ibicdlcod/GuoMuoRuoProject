/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "clientv2.h"
#include <QSettings>

using namespace std::chrono_literals;

extern std::unique_ptr<QSettings> settings;

void Client::chooseHomePort(KP::AllegianceGroup nation)
{
    QByteArray msg = KP::clientHomePort(nation);
    sender->enqueue(msg);
}

void Client::chooseNode(int mapId, int chosenNodeId) {
    if(!isInBattle()) {
        //% "You can't enter a sortie map illegally!"
        qWarning() << qtTrId("illegal-map-progress");
        return;
    }
    QByteArray msg = KP::clientChooseNode(mapId, chosenNodeId);
    sender->enqueue(msg);
    socket.flush();
}

void Client::initARDPurchase(int packageId) {
    if(!loggedIn()) {
        return;
    }
    //% "Starting ARD coupon purchase..."
    qInfo() << qtTrId("ard-purchase-start");
    QByteArray msg = KP::clientInitARDPurchase(packageId);
    sender->enqueue(msg);
    socket.flush();
}

void Client::onMicroTxnAuth(MicroTxnAuthorizationResponse_t *pParam) {
    if(pParam->m_unAppID != (uint32)KP::steamAppId) {
        return;
    }
    QByteArray msg = KP::clientARDPurchaseAuth(
        pParam->m_ulOrderID,
        pParam->m_bAuthorized != 0);
    QMetaObject::invokeMethod(this, [this, msg]() {
        if(sender == nullptr) return;
        sender->enqueue(msg);
        socket.flush();
    }, Qt::QueuedConnection);
}

void Client::sortie(int mapId, int fleetIndex, bool isExpedition) {
    QByteArray msg = KP::clientSortie(mapId, fleetIndex, isExpedition);
    sender->enqueue(msg);
}

void Client::queryNextNode(int mapId, int prevNode, bool retreat) {
    if(!isInBattle()) {
        //% "You can't enter a sortie map illegally!"
        qWarning() << qtTrId("illegal-map-progress");
        return;
    }
    else {
        QByteArray msg = KP::clientQueryNextNode(mapId, prevNode, retreat);
        sender->enqueue(msg);
        socket.flush();
    }
}

void Client::doBattle(const QJsonObject &contents) {
    QByteArray msg = KP::clientDoBattleNode(contents);
    sender->enqueue(msg);
}

void Client::sendFleetData(const QJsonArray &content) {
    QByteArray msg = KP::clientFleetData(content);
    sender->enqueue(msg);
    socket.flush();
}

void Client::doAddEquip(const QStringList &cmdParts) {
    if(cmdParts.length() < 2) {
        //% "Usage: addequip [equipid]"
        emit qout(qtTrId("addequip-usage"));
        return;
    }
    else {
        int equipid = cmdParts[1].toInt(nullptr, 0);
        if(equipid == 0) {
            //% "Equipment id invalid."
            emit qout(qtTrId("develop-invalid-id"));
            return;
        }
        QByteArray msg = KP::clientAddEquip(equipid);
        sender->enqueue(msg);
        return;
    }
}

/* Develop equipment */
void Client::doDevelop(const QStringList &cmdParts) {
    if(cmdParts.length() < 3) {
        //% "Usage: develop [equipid] [FactorySlot]"
        emit qout(qtTrId("develop-usage"));
        return;
    }
    else {
        int equipid = cmdParts[1].toInt(nullptr, 0);
        if(equipid == 0) {
            //% "Equipment id invalid."
            emit qout(qtTrId("develop-invalid-id"));
            return;
        }
        int factoSlot = cmdParts[2].toInt();
        QByteArray msg = KP::clientDevelop(equipid, false, factoSlot);
        sender->enqueue(msg);
        return;
    }
}

void Client::doBuyEquip(int equipDef) {
    if(equipDef == 0) {
        return;
    }
    QByteArray msg = KP::clientBuy(equipDef);
    sender->enqueue(msg);
}

void Client::doBuyFromStore(int equipDef) {
    if(equipDef == 0) {
        return;
    }
    QByteArray msg = KP::clientBuyFromStore(equipDef);
    sender->enqueue(msg);
}

void Client::doConstructShip(int shipDef, const QList<QUuid> &defaultEquips,
                               QUuid shipToRemodel,
                               int factoryID) {
    if(shipDef == 0) {
        return;
    }
    QByteArray msg = KP::clientConstruct(shipDef,
                                         defaultEquips,
                                         shipToRemodel,
                                         factoryID);
    sender->enqueue(msg);
}

/* Get developed equipment */
void Client::doFetch(const QStringList &cmdParts) {
    if(cmdParts.length() < 2) {
        //% "Usage: fetch [FactorySlot]"
        emit qout(qtTrId("fetch-usage"));
        return;
    }
    else {
        int factoSlot = cmdParts[1].toInt();
        QByteArray msg = KP::clientFetch(factoSlot);
        sender->enqueue(msg);
        return;
    }
}

void Client::doForceFetch(int slotnum) {
    QByteArray msg = KP::clientFetch(slotnum, true);
    sender->enqueue(msg);
}

void Client::doForceRepair(int slotnum) {
    QByteArray msg = KP::clientDemandRepair(QUuid(), slotnum, false, true);
    sender->enqueue(msg);
    socket.flush();
}

void Client::doDestructEquip(const QList<QUuid> &trash) {
    if(trash.empty())
        return;
    else {
        QByteArray msg = KP::clientDemandDestructEquip(trash);
        sender->enqueue(msg);
    }
}

void Client::doImproveEquip(const QList<QUuid> &candidates) {
    if(candidates.empty())
        return;
    else {
        QByteArray msg = KP::clientDemandModernize(candidates, true);
        sender->enqueue(msg);
    }
}

void Client::doModernizeShip(const QList<QUuid> &candidates) {
    if(candidates.empty())
        return;
    else {
        QByteArray msg = KP::clientDemandModernize(candidates, false);
        sender->enqueue(msg);
    }
}

void Client::doRefreshDock() {
    QByteArray msg = KP::clientDockRefresh();
    sender->enqueue(msg);
    socket.flush();
}

/* Request current factory state to server */
void Client::doRefreshFactory() {
    QByteArray msg = KP::clientFactoryRefresh();
    sender->enqueue(msg);
    socket.flush();
}

void Client::doRefreshFactoryAnchorage() {
    QByteArray msg = KP::clientDemandShipInfoUser();
    sender->enqueue(msg);
    socket.flush();
}

void Client::doRefreshFactoryArsenal() {
    QByteArray msg = KP::clientDemandEquipInfoUser();
    sender->enqueue(msg);
    socket.flush();
}

void Client::doRefreshRank(int rowsPerPage,
                             std::optional<int> pageNum) {
    QByteArray msg = KP::clientDemandRankInfo(rowsPerPage, pageNum);
    sender->enqueue(msg);
    socket.flush();
}

void Client::doRepair(const QUuid &uuid, int slotnum) {
    QByteArray msg = KP::clientDemandRepair(uuid, slotnum, false, false);
    sender->enqueue(msg);
    socket.flush();
}

void Client::doStopRepair(int slotnum) {
    QByteArray msg = KP::clientDemandRepair(QUuid(), slotnum, true, false);
    sender->enqueue(msg);
    socket.flush();
}

void Client::stopRepair(int slotnum) {
    qCritical() << slotnum;
}

/* Admin delete all equips */
void Client::doDeleteTestEquip() {
    QByteArray msg = KP::clientAdminTestEquipRemove();
    sender->enqueue(msg);
}

/* Admin delete all ships */
void Client::doDeleteTestShip() {
    QByteArray msg = KP::clientAdminTestShipRemove();
    sender->enqueue(msg);
}

/* Admin generate a bunch of test equips */
void Client::doGenerateTestEquip() {
    QByteArray msg = KP::clientAdminTestEquip();
    sender->enqueue(msg);
}

/* Admin generate a bunch of test ships */
void Client::doGenerateTestShip() {
    QByteArray msg = KP::clientAdminTestShip();
    sender->enqueue(msg);
}

Equipment * Client::getEquipmentReg(int equipid) {
    if(!equipRegistryCache.contains(equipid))
        return new Equipment(0, this);
    else
        return equipRegistryCache.value(equipid);
}

Ship * Client::getShipReg(int equipid) {
    if(!shipRegistryCache.contains(equipid))
        return new Ship(0, this);
    else
        return shipRegistryCache.value(equipid);
}

QList<Equipment *> Client::getStoreEquipment() const {
    QList<Equipment *> result;
    for(Equipment *equip : std::as_const(equipRegistryCache)) {
        if(equip->availableInStore()) {
            result.append(equip);
        }
    }
    return result;
}
