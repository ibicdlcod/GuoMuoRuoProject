/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "clientv2.h"
#include <QSettings>
#include <QEventLoop>
#include <QTimer>

using namespace std::chrono_literals;

extern std::unique_ptr<QSettings> settings;

void Clientv2::demandEquipCache() {
    QDateTime localCacheTimeStamp = settings->value("client/equipdbtimestamp",
                                                    QDateTime(QDate(1970,01,01),
                                                              QTime(0, 0, 0))
                                                    ).toDateTime();
    QByteArray msg = KP::clientDemandEquipInfo(localCacheTimeStamp);
    sender->enqueue(msg);
}

void Clientv2::demandEquipSkillPoints(int equipDef) {
    socket.flush();
    QByteArray msg = KP::clientDemandSkillPoints(equipDef);
    const qint64 written = socket.write(msg);
    if (written <= 0) {
        throw NetworkError(socket.errorString());
    }
    return;
}

void Clientv2::demandMapCache() {
    QDateTime localCacheTimeStamp = settings->value("client/mapdbtimestamp",
                                                    QDateTime(QDate(1970,01,01),
                                                              QTime(0, 0, 0))
                                                    ).toDateTime();
    QByteArray msg = KP::clientDemandMapInfo(localCacheTimeStamp);
    sender->enqueue(msg);
}

void Clientv2::demandMapSupremacy() {
    QByteArray msg = KP::clientDemandMapInfoUser();
    sender->enqueue(msg);
}

void Clientv2::demandShipCache() {
    QDateTime localCacheTimeStamp = settings->value("client/shipdbtimestamp",
                                                    QDateTime(QDate(1970,01,01),
                                                              QTime(0, 0, 0))
                                                    ).toDateTime();
    QByteArray msg = KP::clientDemandShipInfo(localCacheTimeStamp);
    sender->enqueue(msg);
}

void Clientv2::updateEquipCache(const QJsonObject &input) {
    QJsonObject cachedInput;
    if(!input.contains("content")) {
        cachedInput
            = settings->value("client/equipdbcache").toJsonObject();
    }
    else {
        settings->setValue("client/equipdbtimestamp",
                           QDateTime::fromString(
                               input["timestamp"].toString()));
        settings->setValue("client/equipdbcache",
                           input);
        cachedInput = input;
    }
    QJsonArray equipDefs = cachedInput["content"].toArray();
    for(auto equipDef: equipDefs) {
        QJsonObject equipDValue = equipDef.toObject();
        int eid = equipDValue.value("eid").toInt();
        equipRegistryCache[eid] = new Equipment(equipDValue, this);
    }

    //% "Equipment cache length: %1"
    qDebug() << qtTrId("equipment-cache-length")
                    .arg(Clientv2::getInstance()
                             .equipRegistryCache.size());

    equipRegistryCacheGood = true;
    emit equipRegistryComplete();
}

void Clientv2::updateShipCache(const QJsonObject &input) {
    QJsonObject cachedInput;
    if(!input.contains("content")) {
        cachedInput
            = settings->value("client/shipdbcache").toJsonObject();
    }
    else {
        settings->setValue("client/shipdbtimestamp",
                           QDateTime::fromString(
                               input["timestamp"].toString()));
        settings->setValue("client/shipdbcache",
                           input);
        cachedInput = input;
    }
    QJsonArray shipDefs = cachedInput["content"].toArray();
    for(auto shipDef: shipDefs) {
        QJsonObject shipDValue = shipDef.toObject();
        int sid = shipDValue.value("sid").toInt();
        shipRegistryCache[sid] = new Ship(shipDValue, this);
    }

    //% "Ship cache length: %1"
    qDebug() << qtTrId("shipment-cache-length")
                    .arg(Clientv2::getInstance()
                             .shipRegistryCache.size());

    shipRegistryCacheGood = true;
    emit shipRegistryComplete();
}

void Clientv2::updateMapCache(const QJsonObject &input) {
    QJsonObject cachedInput;
    if(!input.contains("content")) {
        cachedInput
            = settings->value("client/mapdbcache").toJsonObject();
    }
    else {
        settings->setValue("client/mapdbtimestamp",
                           QDateTime::fromString(
                               input["timestamp"].toString()));
        settings->setValue("client/mapdbcache",
                           input);
        cachedInput = input;
    }
    QJsonArray mapDefs = cachedInput["content"].toArray();
    for(auto mapDef: mapDefs) {
        QJsonObject mapDValue = mapDef.toObject();
        int eid = mapDValue.value("id").toInt()
                  + KP::mapIDDifficultyMask * mapDValue.value("diff").toInt();
        mapRegistryCache[eid] = new MapWithDiff(mapDValue);
    }

    //% "Map cache length: %1"
    qDebug() << qtTrId("map-cache-length")
                    .arg(Clientv2::getInstance()
                             .mapRegistryCache.size());

    mapRegistryCacheGood = true;
    emit mapRegistryComplete();
}

/* Not generalized because used as slots */
void Clientv2::switchToFactory() {
    if(!loggedIn()) {
        emit qout(qtTrId("access-denied-login-first"));
        return;
    }
    if(gameState == KP::Factory) {
        return;
    } else {
        gameState = KP::Factory;
        emit gamestateChanged(KP::Factory);
        if(!equipRegistryCacheGood) {
            demandEquipCache();
        }
        if(!shipRegistryCacheGood) {
            demandShipCache();
        }
    }
}

void Clientv2::switchToRepairView() {
    if(!loggedIn()) {
        emit qout(qtTrId("access-denied-login-first"));
        return;
    }
    if(gameState == KP::RepairView) {
        return;
    } else {
        gameState = KP::RepairView;
        emit gamestateChanged(KP::RepairView);
    }
}

void Clientv2::switchToBattleView() {
    if(!loggedIn()) {
        emit qout(qtTrId("access-denied-login-first"));
        return;
    }
    if(gameState == KP::SortieMapView) {
        return;
    } else {
        gameState = KP::SortieMapView;
        emit gamestateChanged(KP::SortieMapView);
    }
}

void Clientv2::switchToFleetView() {
    if(!loggedIn()) {
        emit qout(qtTrId("access-denied-login-first"));
        return;
    }
    if(gameState == KP::FleetView) {
        return;
    } else {
        gameState = KP::FleetView;
        emit gamestateChanged(KP::FleetView);
    }
}

void Clientv2::switchToTech() {
    if(!loggedIn()) {
        emit qout(qtTrId("access-denied-login-first"));
        return;
    }
    if(gameState == KP::TechView) {
        return;
    } else {
        gameState = KP::TechView;
        emit gamestateChanged(KP::TechView);
        if(equipRegistryCacheGood && shipRegistryCacheGood) {
            socket.flush();
            QByteArray msg = KP::clientDemandTech(0);
            sender->enqueue(msg);
            socket.flush();
        }
        else if(!equipRegistryCacheGood){
            demandEquipCache();
            connect(this, &Clientv2::equipRegistryComplete,
                    this, &Clientv2::demandShipCache);
            connect(this, &Clientv2::shipRegistryComplete,
                    this, &Clientv2::switchToTech2);
        }
        else{
            demandShipCache();
            connect(this, &Clientv2::shipRegistryComplete,
                    this, &Clientv2::switchToTech2);
        }
    }
}

void Clientv2::switchToTech2() {
    socket.flush();
    QByteArray msg = KP::clientDemandTech(0);
    sender->enqueue(msg);
    socket.flush();
}

void Clientv2::switchToTech3(int techId) {
    socket.flush();
    QByteArray msg = KP::clientDemandTech(techId);
    sender->enqueue(msg);
    socket.flush();
}

void Clientv2::tsunkitAssets() {
    QSet<int> iconGroups;
    for(auto equip: std::as_const(equipRegistryCache)) {
        iconGroups.insert(equip->type.iconGroup());
    }
    downloadRequired = iconGroups.size();
    for(auto iconGroup: iconGroups)  {
        ResourceFetch *resourceFetcher = new ResourceFetch();
        resourceFetcher->downloadFile(
            QString("https://tsunkit.net/api/assets/images/equipTypeIcons/%1").arg(iconGroup),
            QString("%1.png").arg(iconGroup),
            QStringLiteral("equipTypeIcons/"));

        // Connect the signal you're waiting for to the QEventLoop::quit slot
        connect(resourceFetcher, &ResourceFetch::finished, this, [this]()
                {
                    downloadCompleted += 1;
                    QObject::sender()->deleteLater();
                    if(downloadCompleted == downloadRequired) {
                        downloadCompleted = 0;
                        tsunkitAssets2();
                    }
                });
    }
}

void Clientv2::tsunkitAssets2() {
    QSet<int> oldInternalIDs;
    for(auto ship: std::as_const(shipRegistryCache)) {
        oldInternalIDs.insert(ship->attr["OldInternalNo."]);
    }
    downloadRequired = oldInternalIDs.size();
    int downloadStarted = 0;
    for(auto oldInternalID: oldInternalIDs) {
        if(oldInternalID == 0) {
            downloadRequired -= 1;
            continue;
        }
        while(downloadStarted - downloadCompleted > 200) {
            QEventLoop loop;
            QTimer::singleShot(100ms, &loop, &QEventLoop::quit);
            loop.exec();
        }
        ResourceFetch *resourceFetcher = new ResourceFetch();
        downloadStarted += 1;
        resourceFetcher->downloadFile(
            QString("https://tsunkit.net/api/assets/images/shipIcons/%1_100").arg(oldInternalID),
            QString("%1.png").arg(oldInternalID),
            QStringLiteral("shipIcons/"));

        connect(resourceFetcher, &ResourceFetch::finished, this, [this]()
                {
                    downloadCompleted += 1;
                    QObject::sender()->deleteLater();
                    if(downloadCompleted == downloadRequired) {
                        downloadCompleted = 0;
                        qInfo() << "Download success!";
                        emit tsunkitAssetsComplete();
                    }
                });
    }
}

void Clientv2::sendTestMessages() {
#pragma message(NOT_M_CONST)
    /*
    constexpr int size = 20;
    for(int i = 0; i < size; ++i) {
        QByteArray msg = KP::clientTestMessages(i);
        sender->enqueue(msg);
    }
*/
    QSet<int> iconGroups;
    for(auto equip: std::as_const(equipRegistryCache)) {
        iconGroups.insert(equip->type.iconGroup());
    }
    for(auto iconGroup: iconGroups)  {
        resourceFetcher.downloadFile(
            QString("https://tsunkit.net/api/assets/images/equipTypeIcons/%1").arg(iconGroup),
            QString("%1.png").arg(iconGroup),
            QStringLiteral("equipTypeIcons/"));
        QEventLoop loop;
        QTimer timer; // Optional: for timeout
        timer.setSingleShot(true); // Ensure timer only fires once

        // Connect the signal you're waiting for to the QEventLoop::quit slot
        connect(&resourceFetcher, &ResourceFetch::finished, &loop, &QEventLoop::quit);

        timer.start(settings->value("networkclient/downloadwaittimemsec", 80000).toInt());

        loop.exec();
    }
}
