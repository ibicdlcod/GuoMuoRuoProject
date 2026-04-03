/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "clientv2.h"

#include <QCoreApplication>
#include <QEventLoop>
#include <QFile>
#include <QSettings>
#include <QTextStream>
#include <QTimer>

#include "networkerror.h"

using namespace std::chrono_literals;

extern std::unique_ptr<QSettings> settings;

void Client::demandEquipCache() {
    QDateTime localCacheTimeStamp = settings->value("client/equipdbtimestamp",
                                                    QDateTime(QDate(1970,01,01),
                                                              QTime(0, 0, 0))
                                                    ).toDateTime();
    QByteArray msg = KP::clientDemandEquipInfo(localCacheTimeStamp);
    sender->enqueue(msg);
}

void Client::demandEquipSkillPoints(int equipDef) {
    socket.flush();
    QByteArray msg = KP::clientDemandSkillPoints(equipDef);
    const qint64 written = socket.write(msg);
    if (written <= 0) {
        throw NetworkError(socket.errorString());
    }
    return;
}

void Client::demandMapCache() {
    QDateTime localCacheTimeStamp = settings->value("client/mapdbtimestamp",
                                                    QDateTime(QDate(1970,01,01),
                                                              QTime(0, 0, 0))
                                                    ).toDateTime();
    QByteArray msg = KP::clientDemandMapInfo(localCacheTimeStamp);
    sender->enqueue(msg);
}

void Client::demandMapSupremacy() {
    QByteArray msg = KP::clientDemandMapInfoUser();
    sender->enqueue(msg);
}

/* 6.2-supremacy.md#Resource_gain
 * Compute resource gain client-side using Map_nodes.csv base values,
 * Map_relations.csv RS edges (loaded at startup into resourceMapBases /
 * resourceMapLinks), and cached mapSupremacies.
 * WARNING: displayed values may be nonsensical if the client-side CSV
 * files differ from the server's copies. */
void Client::demandResourceGain() {
    QJsonObject result;
    for(auto it = resourceMapBases.constBegin();
        it != resourceMapBases.constEnd(); ++it) {
        int resMapId = it.key();
        const QMap<QString, double> &bases = it.value();
        QList<int> seaMaps = resourceMapLinks.values(resMapId);
        int count = seaMaps.size();
        double totalSup = 0.0;
        for(int seaId : seaMaps) {
            double sup = mapSupremacies.value(seaId, 0.0);
            if(sup > 0.0) {
                totalSup += sup;
            }
        }
        double avgSup = (count > 0) ? totalSup / count : 0.0;
        QJsonObject entry;
        entry[QStringLiteral("supremacy")] = avgSup;
        for(auto rit = bases.constBegin(); rit != bases.constEnd(); ++rit) {
            /* avgSup is in percentage units (0–300), not a fraction;
             * matches server: SUM(base * (1/count) * supremacy_i / ctrl) */
            entry[rit.key()] = avgSup * rit.value() / 1000.0;
        }
        result[QString::number(resMapId)] = entry;
    }
    emit receivedResourceGainInfo(result);
}

void Client::demandShipCache() {
    QDateTime localCacheTimeStamp = settings->value("client/shipdbtimestamp",
                                                    QDateTime(QDate(1970,01,01),
                                                              QTime(0, 0, 0))
                                                    ).toDateTime();
    QByteArray msg = KP::clientDemandShipInfo(localCacheTimeStamp);
    sender->enqueue(msg);
}

void Client::updateEquipCache(const QJsonObject &input) {
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
                    .arg(Client::getInstance()
                             .equipRegistryCache.size());

    equipRegistryCacheGood = true;
    emit equipRegistryComplete();
}

void Client::updateShipCache(const QJsonObject &input) {
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
                    .arg(Client::getInstance()
                             .shipRegistryCache.size());

    shipRegistryCacheGood = true;
    emit shipRegistryComplete();
}

void Client::updateMapCache(const QJsonObject &input) {
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
                    .arg(Client::getInstance()
                             .mapRegistryCache.size());

    mapRegistryCacheGood = true;
    emit mapRegistryComplete();
}

/* 8.1-supply.md#Supply_chain_and_attrition
 * Load undirected supply chain edges from Map_relations.csv.
 * Only rows whose Type is not "RS" are supply chain edges.
 * RS rows link resource virtual maps to coastal maps and
 * are not part of the supply routing graph. */
void Client::loadSupplyChain() {
    QString path = QCoreApplication::applicationDirPath()
                   + "/Map_relations.csv";
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        //% "Map_relations.csv not found: %1"
        qWarning() << qtTrId("map-relations-not-found")
                          .arg(file.errorString());
        return;
    }
    QTextStream in(&file);
    QString header = in.readLine(); /* skip header row */
    Q_UNUSED(header)
    while(!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if(line.isEmpty()) {
            continue;
        }
        QStringList parts = line.split(',');
        if(parts.size() < 3) {
            continue;
        }
        QString type = parts[0].trimmed();
        if(type == "RS") {
            continue; /* resource-map links, not supply routes */
        }
        bool okA = false, okB = false;
        int a = parts[1].trimmed().toInt(&okA);
        int b = parts[2].trimmed().toInt(&okB);
        if(okA && okB) {
            supplyChainEdges.append({a, b});
        }
    }
}

/* 6.2-supremacy.md#Resource_gain
 * Load resource base values from Map_nodes.csv (rows with ID > 1024)
 * and RS edges from Map_relations.csv into resourceMapBases /
 * resourceMapLinks for client-side resource gain display. */
void Client::loadResourceMaps() {
    /* --- Map_nodes.csv ------------------------------------------ */
    static const QStringList csvAttrs = {
        QStringLiteral("O"), QStringLiteral("E"), QStringLiteral("S"),
        QStringLiteral("A"), QStringLiteral("R"), QStringLiteral("W"),
        QStringLiteral("C")
    };
    QString nodesPath = QCoreApplication::applicationDirPath()
                        + "/Map_nodes.csv";
    QFile nodesFile(nodesPath);
    if(!nodesFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        //% "Map_nodes.csv not found: %1"
        qWarning() << qtTrId("map-nodes-not-found")
                          .arg(nodesFile.errorString());
    } else {
        QTextStream in(&nodesFile);
        in.readLine(); /* descriptive header row  */
        in.readLine(); /* column-name header row  */
        while(!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if(line.isEmpty()) {
                continue;
            }
            QStringList parts = line.split(',');
            if(parts.size() < 13) {
                continue;
            }
            bool ok = false;
            int id = parts[0].trimmed().toInt(&ok);
            if(!ok || id <= 1024) {
                continue; /* regular sea maps have no resource columns */
            }
            QMap<QString, double> bases;
            for(int i = 0; i < csvAttrs.size(); ++i) {
                QString val = parts[6 + i].trimmed();
                if(!val.isEmpty()) {
                    bases[csvAttrs[i]] = val.toDouble();
                }
            }
            if(!bases.isEmpty()) {
                resourceMapBases[id] = bases;
            }
        }
    }

    /* --- Map_relations.csv (RS entries only) -------------------- */
    QString relPath = QCoreApplication::applicationDirPath()
                      + "/Map_relations.csv";
    QFile relFile(relPath);
    if(!relFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return; /* already warned in loadSupplyChain() */
    }
    QTextStream rin(&relFile);
    rin.readLine(); /* skip header */
    while(!rin.atEnd()) {
        QString line = rin.readLine().trimmed();
        if(line.isEmpty()) {
            continue;
        }
        QStringList parts = line.split(',');
        if(parts.size() < 3 || parts[0].trimmed() != QStringLiteral("RS")) {
            continue;
        }
        bool okA = false, okB = false;
        int a = parts[1].trimmed().toInt(&okA);
        int b = parts[2].trimmed().toInt(&okB);
        if(!okA || !okB) {
            continue;
        }
        /* node1 is the resource map (>1024), node2 the sea map */
        if(a > 1024) {
            resourceMapLinks.insert(a, b);
        } else {
            resourceMapLinks.insert(b, a);
        }
    }
}

/* Not generalized because used as slots */
void Client::switchToFactory() {
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

void Client::switchToRepairView() {
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

void Client::switchToBattleView() {
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

void Client::switchToFleetView() {
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

void Client::switchToTech() {
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
            connect(this, &Client::equipRegistryComplete,
                    this, &Client::demandShipCache);
            connect(this, &Client::shipRegistryComplete,
                    this, &Client::switchToTech2);
        }
        else{
            demandShipCache();
            connect(this, &Client::shipRegistryComplete,
                    this, &Client::switchToTech2);
        }
    }
}

void Client::switchToTech2() {
    socket.flush();
    QByteArray msg = KP::clientDemandTech(0);
    sender->enqueue(msg);
    socket.flush();
}

void Client::switchToTech3(int techId) {
    socket.flush();
    QByteArray msg = KP::clientDemandTech(techId);
    sender->enqueue(msg);
    socket.flush();
}

void Client::tsunkitAssets() {
    QSet<int> iconGroups;
    for(auto equip: std::as_const(equipRegistryCache)) {
        iconGroups.insert(equip->type.iconGroup());
    }
    downloadRequired = iconGroups.size();
    for(auto iconGroup: iconGroups)  {
        ResourceFetch *resourceFetcher = new ResourceFetch();
        resourceFetcher->downloadFile(
            QString("https://tsunkit.net/api/assets/images/equipTypeIcons/%1")
                .arg(iconGroup),
            QString("%1.png").arg(iconGroup),
            QStringLiteral("TsunkitMode/equipTypeIcons/"));

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

void Client::tsunkitAssets2() {
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
            QString("https://tsunkit.net/api/assets/images/shipIcons/%1_100")
                .arg(oldInternalID),
            QString("%1.png").arg(oldInternalID),
            QStringLiteral("TsunkitMode/shipIcons/"));

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

void Client::sendTestMessages() {
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
            QString("https://tsunkit.net/api/assets/images/equipTypeIcons/%1")
                .arg(iconGroup),
            QString("%1.png").arg(iconGroup),
            QStringLiteral("TsunkitMode/equipTypeIcons/"));
        QEventLoop loop;
        QTimer timer; // Optional: for timeout
        timer.setSingleShot(true); // Ensure timer only fires once

        // Connect the signal you're waiting for to the QEventLoop::quit slot
        connect(&resourceFetcher, &ResourceFetch::finished,
                &loop, &QEventLoop::quit);

        timer.start(settings->value(
            "networkclient/downloadwaittimemsec", 80000).toInt());

        loop.exec();
    }
}
