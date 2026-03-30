/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#define NOMINMAX
#include "server.h"

#include <QFile>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>

#include "../Protocol/equiptype.h"
#include "kerrors.h"

QT_BEGIN_NAMESPACE

bool Server::importEquipFromCSV() {
    QSqlDatabase db = QSqlDatabase::database();
    if(!db.isValid()) {
        throw DBError(qtTrId("database-uninit"));
        return false;
    }

    QString csvFileName =
            settings->value("server/equip_reg_csv", "Equip.csv").toString();
    QFile *csvFile = new QFile(csvFileName);
    if(Q_UNLIKELY(!csvFile) || !csvFile->open(QIODevice::ReadOnly)) {
        //% "%1: CSV file cannot be opened"
        qCritical() << qtTrId("bad-csv").arg(csvFileName);
        return false;
    }

    QTextStream textStream(csvFile);
    QString titleIndicator = textStream.readLine();
    QStringList indicatorParts = titleIndicator.split(",");
    QString title = textStream.readLine();
    QStringList titleParts = title.split(",");

    db.transaction();

    QSqlQuery insertEquipName;
    insertEquipName.prepare(
        "INSERT OR REPLACE INTO EquipName "
        "(EquipID) "
        "VALUES (:id);");

    QSqlQuery replaceEquipReg;
    replaceEquipReg.prepare(
        "REPLACE INTO EquipReg "
        "(EquipID, Attribute, Intvalue) "
        "VALUES (:id, :attr, :value);");

    QHash<QString, QSqlQuery> updateEquipLang;
    for(int i = 0; i < titleParts.length(); ++i) {
        if(indicatorParts[i].compare("name", Qt::CaseInsensitive) == 0) {
            QString lang = titleParts[i];
            if(!lang.isEmpty() && !updateEquipLang.contains(lang)) {
                updateEquipLang[lang].prepare(
                    "UPDATE EquipName "
                    "SET " + lang + " = :value "
                    "WHERE EquipID = :id;");
            }
        }
    }

    int importedEquips = 0;
    while(!textStream.atEnd()) {
        QString text = textStream.readLine();
        if(text.startsWith(","))
            continue;
        else {
            QStringList lineParts = text.split(",");
            int equipid = lineParts[indicatorParts.indexOf("id")].toInt();
            if(lineParts.size() < 7) {
                //% "incomplete equip type definition"
                qCritical() << qtTrId("equip-def-incomplete");
            }
            else {
                int type = EquipType::strToIntRep(lineParts[3]);
                if(type == 0 && !lineParts[1].isEmpty()) {
                    qWarning() << lineParts[0]
                            << "\tUnsupported type: " << lineParts[3];
                }
                insertEquipName.bindValue(":id", equipid);
                if(!insertEquipName.exec()) {
                    db.rollback();
                    //% "Import equipment database failed!"
                    throw DBError(qtTrId("equip-import-failed"),
                                  insertEquipName.lastError(),
                                  insertEquipName.lastQuery());
                    return false;
                }
                for(int i = 0; i < titleParts.length(); ++i) {
                    if(indicatorParts[i].compare("name", Qt::CaseInsensitive)
                            == 0) {
                        auto &q = updateEquipLang[titleParts[i]];
                        q.bindValue(":id", equipid);
                        q.bindValue(":value", lineParts[i]);
                        if(!q.exec()) {
                            db.rollback();
                            //% "Import equipment database failed!"
                            throw DBError(qtTrId("equip-import-failed"),
                                          q.lastError(), q.lastQuery());
                            return false;
                        }
                    }
                    else if(indicatorParts[i].compare("type",
                                                      Qt::CaseInsensitive)
                            == 0) {
                        replaceEquipReg.bindValue(":id", equipid);
                        replaceEquipReg.bindValue(":attr", titleParts[i]);
                        replaceEquipReg.bindValue(":value",
                                        EquipType::strToIntRep(lineParts[i]));
                        if(!replaceEquipReg.exec()) {
                            db.rollback();
                            throw DBError(qtTrId("equip-import-failed"),
                                          replaceEquipReg.lastError(),
                                          replaceEquipReg.lastQuery());
                            return false;
                        }
                    }
                    else if(indicatorParts[i].compare("attr",
                                                      Qt::CaseInsensitive)
                            == 0){
                        replaceEquipReg.bindValue(":id", equipid);
                        replaceEquipReg.bindValue(":attr", titleParts[i]);
                        replaceEquipReg.bindValue(":value", lineParts[i].toInt());
                        if(!replaceEquipReg.exec()) {
                            db.rollback();
                            throw DBError(qtTrId("equip-import-failed"),
                                          replaceEquipReg.lastError(),
                                          replaceEquipReg.lastQuery());
                            return false;
                        }
                    }
                }
            }
            importedEquips++;
            if(importedEquips % 10 == 0) {
                //% "Imported %1 equipment(s)"
                qInfo() << qtTrId("num-of-equip-imports")
                           .arg(importedEquips);
            }
        }
    }
    if(!db.commit()) {
        db.rollback();
        //% "Import equipment database failed!"
        throw DBError(qtTrId("equip-import-failed"), db.lastError(), {});
        return false;
    }
    csvFile->close();
    delete csvFile;
    //% "Import equipment registry success!"
    qInfo() << qtTrId("equip-import-good");
    settings->setValue("server/equipdbtimestamp",
                       QDateTime::currentDateTimeUtc());
    return equipmentRefresh();
}

bool Server::importShipFromCSV() {
    QSqlDatabase db = QSqlDatabase::database();
    if(!db.isValid()) {
        throw DBError(qtTrId("database-uninit"));
        return false;
    }

    QString csvFileName =
            settings->value("server/ship_reg_csv", "Ship.csv").toString();
    QFile *csvFile = new QFile(csvFileName);
    if(Q_UNLIKELY(!csvFile) || !csvFile->open(QIODevice::ReadOnly)) {
        //% "%1: CSV file cannot be opened"
        qCritical() << qtTrId("bad-csv").arg(csvFileName);
        return false;
    }

    QTextStream textStream(csvFile);
    QString titleIndicator = textStream.readLine();
    QStringList indicatorParts = titleIndicator.split(",");
    QString title = textStream.readLine();
    QStringList titleParts = title.split(",");

    db.transaction();

    QSqlQuery replaceShipReg;
    replaceShipReg.prepare(
        "REPLACE INTO ShipReg "
        "(ShipID, Attribute, Intvalue) "
        "VALUES (:id, :attr, :value);");

    QSqlQuery replaceShipName;
    replaceShipName.prepare(
        "REPLACE INTO ShipName "
        "(ShipID, lang, textattr, value) "
        "VALUES (:id, :lang, :textattr, :value);");

    int importedShips = 0;
    while(!textStream.atEnd()) {
        QString text = textStream.readLine();
        if(text.startsWith(","))
            continue;
        else {
            QStringList lineParts = text.split(",");
            int shipid = lineParts[indicatorParts.indexOf("id")].toInt();
            if(lineParts.size() < 7)
                qCritical("incomplete ship type definition");
            else {
                for(int i = 0; i < titleParts.length(); ++i) {
                    if(titleParts[i].compare("remodel",
                                             Qt::CaseInsensitive)
                            == 0){
                        replaceShipReg.bindValue(":id", shipid);
                        replaceShipReg.bindValue(":attr", titleParts[i]);
                        replaceShipReg.bindValue(":value",
                            lineParts[i].toInt(nullptr, 16));
                        if(!replaceShipReg.exec()) {
                            db.rollback();
                            //% "Import ship database failed!"
                            throw DBError(qtTrId("ship-import-failed"),
                                          replaceShipReg.lastError(),
                                          replaceShipReg.lastQuery());
                            return false;
                        }
                    }
                    else if(indicatorParts[i].compare("attr",
                                                      Qt::CaseInsensitive)
                            == 0){
                        replaceShipReg.bindValue(":id", shipid);
                        replaceShipReg.bindValue(":attr", titleParts[i]);
                        replaceShipReg.bindValue(":value", lineParts[i].toInt());
                        if(!replaceShipReg.exec()) {
                            db.rollback();
                            throw DBError(qtTrId("ship-import-failed"),
                                          replaceShipReg.lastError(),
                                          replaceShipReg.lastQuery());
                            return false;
                        }
                    }
                    else if(indicatorParts[i].compare("customflags",
                                                      Qt::CaseInsensitive)
                            == 0){
                        if(lineParts[i].isEmpty()) {
                            continue;
                        }
                        replaceShipReg.bindValue(":id", shipid);
                        replaceShipReg.bindValue(":attr", "CUSTOM"+titleParts[i]);
                        replaceShipReg.bindValue(":value", lineParts[i].toInt());
                        if(!replaceShipReg.exec()) {
                            db.rollback();
                            throw DBError(qtTrId("ship-import-failed"),
                                          replaceShipReg.lastError(),
                                          replaceShipReg.lastQuery());
                            return false;
                        }
                    }
                    else if(!indicatorParts[i].isEmpty()
                            && indicatorParts[i].compare(
                                "id", Qt::CaseInsensitive) != 0) {
                        replaceShipName.bindValue(":id", shipid);
                        replaceShipName.bindValue(":lang", titleParts[i]);
                        replaceShipName.bindValue(":textattr", indicatorParts[i]);
                        replaceShipName.bindValue(":value", lineParts[i]);
                        if(!replaceShipName.exec()) {
                            db.rollback();
                            throw DBError(qtTrId("ship-import-failed"),
                                          replaceShipName.lastError(),
                                          replaceShipName.lastQuery());
                            return false;
                        }
                    }
                }
            }
            importedShips++;
            if(importedShips % 10 == 0) {
                //% "Imported %1 ship(s)"
                qInfo() << qtTrId("num-of-ship-imports").arg(importedShips);
            }
        }
    }
    if(!db.commit()) {
        db.rollback();
        //% "Import ship database failed!"
        throw DBError(qtTrId("ship-import-failed"), db.lastError(), {});
        return false;
    }
    csvFile->close();
    delete csvFile;
    //% "Import ship registry success!"
    qInfo() << qtTrId("ship-import-good");
    settings->setValue("server/shipdbtimestamp",
                       QDateTime::currentDateTimeUtc());
    return shipRefresh();
}

bool Server::importMapFromCSV() {
    if(!(importMapNodeFromCSV()
         && importMapRelationFromCSV()
         && importVCRFromCSV())) {
        return false;
    }
    settings->setValue("server/mapdbtimestamp",
                       QDateTime::currentDateTimeUtc());
    return mapRefresh();
}

bool Server::importMapNodeFromCSV() {
    QSqlDatabase db = QSqlDatabase::database();
    if(!db.isValid()) {
        throw DBError(qtTrId("database-uninit"));
        return false;
    }

    QString csvFileName =
            settings->value("server/map_node_reg_csv",
                             "Map_nodes.csv").toString();
    QFile *csvFile = new QFile(csvFileName);
    if(Q_UNLIKELY(!csvFile) || !csvFile->open(QIODevice::ReadOnly)) {
        //% "%1: CSV file cannot be opened"
        qCritical() << qtTrId("bad-csv").arg(csvFileName);
        return false;
    }

    QTextStream textStream(csvFile);
    QString titleIndicator = textStream.readLine();
    QStringList indicatorParts = titleIndicator.split(",");
    QString title = textStream.readLine();
    QStringList titleParts = title.split(",");

    db.transaction();

    {
        QSqlQuery query;
        query.prepare("DELETE FROM MapResource;");
        if(!query.exec()) {
            db.rollback();
            //% "Import map node database failed!"
            throw DBError(qtTrId("map-node-import-failed"),
                          query.lastError(), query.lastQuery());
            return false;
        }
    }

    QSqlQuery replaceMapNode;
    replaceMapNode.prepare(
        "REPLACE INTO MapNode "
        "(MapID) "
        "VALUES (:id);");

    QSqlQuery insertMapResource;
    insertMapResource.prepare(
        "INSERT INTO MapResource "
        "(MapID, Attribute, Intvalue) "
        "VALUES (:id, :attr, :value);");

    QHash<QString, QSqlQuery> updateMapNodeLang;
    for(int i = 0; i < titleParts.length(); ++i) {
        if(indicatorParts[i].compare("name", Qt::CaseInsensitive) == 0) {
            QString lang = titleParts[i];
            if(!lang.isEmpty() && !updateMapNodeLang.contains(lang)) {
                updateMapNodeLang[lang].prepare(
                    "UPDATE MapNode "
                    "SET " + lang + " = :value "
                    "WHERE MapID = :id;");
            }
        }
    }

    int importedMapNodes = 0;
    while(!textStream.atEnd()) {
        QString text = textStream.readLine();
        if(text.startsWith(","))
            continue;
        else {
            QStringList lineParts = text.split(",");
            int mapNodeId = lineParts[indicatorParts.indexOf("id")].toInt();
            replaceMapNode.bindValue(":id", mapNodeId);
            if(!replaceMapNode.exec()) {
                db.rollback();
                //% "Import map node database failed!"
                throw DBError(qtTrId("map-node-import-failed"),
                              replaceMapNode.lastError(),
                              replaceMapNode.lastQuery());
                return false;
            }

            for(int i = 0; i < titleParts.length(); ++i) {
                if(indicatorParts[i].compare("name", Qt::CaseInsensitive)
                        == 0) {
                    auto &q = updateMapNodeLang[titleParts[i]];
                    q.bindValue(":id", mapNodeId);
                    q.bindValue(":value", lineParts[i]);
                    if(!q.exec()) {
                        db.rollback();
                        //% "Import map node database failed!"
                        throw DBError(qtTrId("map-node-import-failed"),
                                      q.lastError(), q.lastQuery());
                        return false;
                    }
                }
                else if(indicatorParts[i].compare("attr", Qt::CaseInsensitive)
                        == 0) {
                    insertMapResource.bindValue(":id", mapNodeId);
                    insertMapResource.bindValue(":attr", titleParts[i]);
                    insertMapResource.bindValue(":value", lineParts[i].toInt());
                    if(!insertMapResource.exec()) {
                        db.rollback();
                        //% "Import map node database failed!"
                        throw DBError(qtTrId("map-node-import-failed"),
                                      insertMapResource.lastError(),
                                      insertMapResource.lastQuery());
                        return false;
                    }
                }
            }
            importedMapNodes++;
            if(importedMapNodes % 10 == 0) {
                //% "Imported %1 map node(s)"
                qInfo() << qtTrId("num-of-map-node-imports")
                           .arg(importedMapNodes);
            }
        }
    }
    if(!db.commit()) {
        db.rollback();
        //% "Import map node database failed!"
        throw DBError(qtTrId("map-node-import-failed"), db.lastError(), {});
        return false;
    }
    csvFile->close();
    delete csvFile;
    //% "Import map node registry success!"
    qInfo() << qtTrId("map-node-import-good");
    settings->setValue("server/mapdbtimestamp",
                       QDateTime::currentDateTimeUtc());
    return true;
}

bool Server::importMapRelationFromCSV() {
    QSqlDatabase db = QSqlDatabase::database();
    if(!db.isValid()) {
        throw DBError(qtTrId("database-uninit"));
        return false;
    }

    QString csvFileName =
            settings->value("server/map_relation_reg_csv",
                             "Map_relations.csv").toString();
    QFile *csvFile = new QFile(csvFileName);
    if(Q_UNLIKELY(!csvFile) || !csvFile->open(QIODevice::ReadOnly)) {
        //% "%1: CSV file cannot be opened"
        qCritical() << qtTrId("bad-csv").arg(csvFileName);
        return false;
    }

    QTextStream textStream(csvFile);
    QString title = textStream.readLine();
    QStringList titleParts = title.split(",");
    Q_UNUSED(titleParts)

    db.transaction();

    {
        QSqlQuery query;
        query.prepare("DELETE FROM MapRelation;");
        if(!query.exec()) {
            db.rollback();
            //% "Import map relation database failed!"
            throw DBError(qtTrId("map-relation-import-failed"),
                          query.lastError(), query.lastQuery());
            return false;
        }
    }

    QSqlQuery replaceMapRelation;
    replaceMapRelation.prepare(
        "REPLACE INTO MapRelation "
        "(Type, Node1, Node2) "
        "VALUES (:type, :id1, :id2);");

    int importedMapRelations = 0;
    while(!textStream.atEnd()) {
        QString text = textStream.readLine();
        if(text.startsWith(","))
            continue;
        else {
            QStringList lineParts = text.split(",");
            replaceMapRelation.bindValue(":type", lineParts[0]);
            replaceMapRelation.bindValue(":id1", lineParts[1].toInt());
            replaceMapRelation.bindValue(":id2", lineParts[2].toInt());
            if(!replaceMapRelation.exec()) {
                db.rollback();
                //% "Import map relation database failed!"
                throw DBError(qtTrId("map-relation-import-failed"),
                              replaceMapRelation.lastError(),
                              replaceMapRelation.lastQuery());
                return false;
            }

            importedMapRelations++;
            if(importedMapRelations % 10 == 0) {
                //% "Imported %1 map relation(s)"
                qInfo() << qtTrId("num-of-map-relation-imports")
                           .arg(importedMapRelations);
            }
        }
    }
    if(!db.commit()) {
        db.rollback();
        //% "Import map relation database failed!"
        throw DBError(qtTrId("map-relation-import-failed"), db.lastError(), {});
        return false;
    }
    csvFile->close();
    delete csvFile;
    //% "Import map relation registry success!"
    qInfo() << qtTrId("map-relation-import-good");
    settings->setValue("server/mapdbtimestamp",
                       QDateTime::currentDateTimeUtc());
    return true;
}

bool Server::importVCRFromCSV() {
    QSqlDatabase db = QSqlDatabase::database();
    if(!db.isValid()) {
        throw DBError(qtTrId("database-uninit"));
        return false;
    }

    QString csvFileName =
            settings->value("server/vcr_reg_csv",
                             "Precondition_relations.csv").toString();
    QFile *csvFile = new QFile(csvFileName);
    if(Q_UNLIKELY(!csvFile) || !csvFile->open(QIODevice::ReadOnly)) {
        //% "%1: CSV file cannot be opened"
        qCritical() << qtTrId("bad-csv").arg(csvFileName);
        return false;
    }

    QTextStream textStream(csvFile);
    QString titleIndicator = textStream.readLine();
    QStringList indicatorParts = titleIndicator.split(",");
    QString title = textStream.readLine();
    QStringList titleParts = title.split(",");

    db.transaction();

    {
        QSqlQuery query;
        query.prepare("DELETE FROM VirtualCondRelation;");
        if(!query.exec()) {
            db.rollback();
            //% "Import vcr database failed!"
            throw DBError(qtTrId("vcr-import-failed"),
                          query.lastError(), query.lastQuery());
            return false;
        }
    }

    QSqlQuery insertVCR;
    insertVCR.prepare(
        "INSERT INTO VirtualCondRelation "
        "(EquipDef, MapDef, MinDiff) "
        "VALUES (:equip, :map, :diff);");

    int importedEquips = 0;
    while(!textStream.atEnd()) {
        QString text = textStream.readLine();
        if(text.startsWith(","))
            continue;
        else {
            QStringList lineParts = text.split(",");
            int equipid = lineParts[indicatorParts.indexOf("id")].toInt();
            for(int i = 0; i < titleParts.length(); ++i) {
                if(indicatorParts[i].compare("id", Qt::CaseInsensitive)
                        == 0) {
                    continue;
                }
                if(indicatorParts[i].compare("name", Qt::CaseInsensitive)
                        == 0) {
                    continue;
                }
                int mapid = indicatorParts[i].toInt();
                int diff = lineParts[i].toInt() - 1;
                if(diff < 0)
                    continue;
                insertVCR.bindValue(":equip", equipid);
                insertVCR.bindValue(":map", mapid);
                insertVCR.bindValue(":diff", diff);
                if(!insertVCR.exec()) {
                    db.rollback();
                    //% "Import vcr database failed!"
                    throw DBError(qtTrId("vcr-import-failed"),
                                  insertVCR.lastError(), insertVCR.lastQuery());
                    return false;
                }
            }
            importedEquips++;
        }
    }
    if(!db.commit()) {
        db.rollback();
        //% "Import vcr database failed!"
        throw DBError(qtTrId("vcr-import-failed"), db.lastError(), {});
        return false;
    }
    csvFile->close();
    delete csvFile;
    //% "Virtual condition relation registry success!"
    qInfo() << qtTrId("vcr-import-good");
    settings->setValue("server/equipdbtimestamp",
                       QDateTime::currentDateTimeUtc());
    return true;
}

bool Server::exportEquipToCSV() const {
    QString csvFileName =
        settings->value("server/equip_reg_csv", "Equip.csv").toString();
    /* TODO: eliminate raw new/delete when possible */
    QFile *csvFile = new QFile(csvFileName);
    if(Q_UNLIKELY(!csvFile) || !csvFile->open(QIODevice::WriteOnly)) {
        //% "%1: CSV file cannot be opened"
        qCritical() << qtTrId("bad-csv").arg(csvFileName);
        return false;
    }
    QTextStream textStream(csvFile);

    QSqlDatabase db = QSqlDatabase::database();
    if(!db.isValid()) {
        throw DBError(qtTrId("database-uninit"));
        return false;
    }
    QSqlQuery query;
    query.prepare("SELECT * FROM Equip;");
    if(!query.exec()) {
        //% "Load equipment table failed!"
        throw DBError(qtTrId("equip-refresh-failed"),
                      query.lastError(), query.lastQuery());
        return false;
    }
    query.isSelect();
    QSqlRecord rec = query.record();
    for(int i = 0; i < rec.count(); ++i) {
        textStream << rec.fieldName(i) << ",";
    }
    static QRegularExpression rehex("^(EquipID|require2?)$",
                                    QRegularExpression::CaseInsensitiveOption);
    while(query.next()) {
        textStream << "\n";
        for(int i = 0; i < rec.count(); ++i) {
            if(rehex.match(rec.fieldName(i)).hasMatch()) {
                Qt::hex(textStream);
                textStream << "0x" << query.value(i).toInt() << ",";
                Qt::dec(textStream);
            }
            else {
                textStream << query.value(i).toString() << ",";
            }
        }
    }
    csvFile->close();
    delete csvFile;
    //% "Export equipment registry success!"
    qInfo() << qtTrId("equip-export-good");
    return true;
}

bool Server::equipmentRefresh() {
    QSqlDatabase db = QSqlDatabase::database();
    if(!db.isValid()) {
        //% "Database uninitialized!"
        throw DBError(qtTrId("database-uninit"));
        return false;
    }
    QSqlQuery query;
    query.prepare("SELECT EquipID FROM EquipReg;");
    if(!query.exec()) {
        //% "Load equipment table failed!"
        throw DBError(qtTrId("equip-refresh-failed"),
                      query.lastError(), query.lastQuery());
        return false;
    }
    query.isSelect();
    QSqlRecord rec = query.record();
    int idCol = rec.indexOf("EquipID");
    while(query.next()) {
        openEquips.insert(query.value(idCol).toInt());
    }
    equipRegistry.clear();
    for(auto equipID : std::as_const(openEquips)) {
        equipRegistry[equipID] = new Equipment(equipID, this);
    }
    //% "Load equipment registry success!"
    qInfo() << qtTrId("equip-load-good");
    for(auto iter = equipRegistry.constKeyValueBegin();
        iter != equipRegistry.constKeyValueEnd();
        ++iter) {
        generateEquipChilds(iter->first, iter->first);
    }
    //% "Load equipment child list success!"
    qInfo() << qtTrId("equip-child-load-good");
    return true;
}

bool Server::shipRefresh() {
    QSqlDatabase db = QSqlDatabase::database();
    if(!db.isValid()) {
        //% "Database uninitialized!"
        throw DBError(qtTrId("database-uninit"));
        return false;
    }
    QSqlQuery query;
    query.prepare("SELECT DISTINCT ShipID FROM ShipName;");
    if(!query.exec()) {
        //% "Load ship table failed!"
        throw DBError(qtTrId("ship-refresh-failed"),
                      query.lastError(), query.lastQuery());
        return false;
    }
    query.isSelect();
    QSqlRecord rec = query.record();
    int idCol = rec.indexOf("ShipID");
    while(query.next()) {
        openShips.insert(query.value(idCol).toInt());
    }
    shipRegistry.clear();
    for(auto shipID : std::as_const(openShips)) {
        shipRegistry[shipID] = new Ship(shipID, this);
    }
    //% "Load ship registry success!"
    qInfo() << qtTrId("ship-load-good");

    for(auto ship: std::as_const(shipRegistry)) {
        if(ship->isAmnesiac()) {
            continue;
        }
        auto latermodels = ship->getLaterModels(shipRegistry);
        if(!latermodels.empty()) {
            auto latestmodel = *std::max_element(
                latermodels.constBegin(), latermodels.constEnd());
            shipRemodelGroup.insert(latestmodel, ship->getId());
        }
        shipOldIdToNewId[ship->attr["OldInternalNo."]] = ship->getId();
    }
    for(auto shipID: shipRemodelGroup.uniqueKeys()) {
        shipRemodelGroup.insert(shipID, shipID);
    }

    return true;
}

bool Server::mapRefresh()
{
    QSqlDatabase db = QSqlDatabase::database();
    if(!db.isValid()) {
        //% "Database uninitialized!"
        throw DBError(qtTrId("database-uninit"));
        return false;
    }
    QSqlQuery query;
    query.prepare("SELECT DISTINCT MapID FROM MapNode;");
    if(!query.exec()) {
        //% "Load map table failed!"
        throw DBError(qtTrId("map-refresh-failed"),
                      query.lastError(), query.lastQuery());
        return false;
    }
    query.isSelect();
    QSqlRecord rec = query.record();
    int idCol = rec.indexOf("MapID");
    while(query.next()) {
        int mapID = query.value(idCol).toInt();
        {
            QSqlQuery query;
            query.prepare("SELECT Attribute, Intvalue FROM MapResource "
                          "WHERE MapID = :id;");
            query.bindValue(":id", mapID);
            if(!query.exec()) {
                //% "Load map table failed!"
                throw DBError(qtTrId("map-refresh-failed"),
                              query.lastError(), query.lastQuery());
                return false;
            }
            query.isSelect();
            QSqlRecord rec = query.record();
            int attrCol = rec.indexOf("Attribute");
            int valueCol = rec.indexOf("Intvalue");
            int O = 0, E = 0, S = 0, A = 0, R = 0, W = 0, C = 0;
            while(query.next()) {
                QString attr = query.value(attrCol).toString();
                int val = query.value(valueCol).toInt();
                if(attr.compare("O", Qt::CaseInsensitive)) {
                    O = val;
                }
                if(attr.compare("E", Qt::CaseInsensitive)) {
                    E = val;
                }
                if(attr.compare("S", Qt::CaseInsensitive)) {
                    S = val;
                }
                if(attr.compare("R", Qt::CaseInsensitive)) {
                    R = val;
                }
                if(attr.compare("A", Qt::CaseInsensitive)) {
                    A = val;
                }
                if(attr.compare("W", Qt::CaseInsensitive)) {
                    W = val;
                }
                if(attr.compare("C", Qt::CaseInsensitive)) {
                    C = val;
                }
            }
            resourceMaps[mapID] = ResOrd(O, E, S, R, A, W, C);
        }
        if(mapID < KP::resourceMapIDStart) {
            int x = 0;
            int y = 0;
            {
                QSqlQuery query;
                query.prepare("SELECT Attribute, Intvalue FROM MapResource "
                              "WHERE MapID = :id "
                              "AND (Attribute = 'x' OR Attribute = 'y');");
                query.bindValue(":id", mapID);
                if(!query.exec()) {
                    //% "Load map table failed!"
                    throw DBError(qtTrId("map-refresh-failed"),
                                  query.lastError(), query.lastQuery());
                    return false;
                }
                query.isSelect();
                QSqlRecord rec = query.record();
                int attrCol = rec.indexOf("Attribute");
                int valueCol = rec.indexOf("Intvalue");
                while(query.next()) {
                    QString attr2 = query.value(attrCol).toString();
                    if(attr2.compare("x", Qt::CaseInsensitive) == 0) {
                        x = query.value(valueCol).toInt();
                    }
                    if(attr2.compare("y", Qt::CaseInsensitive) == 0) {
                        y = query.value(valueCol).toInt();
                    }
                }
            }
            Map m{mapID, x, y, QMap<int, MapNode>()};
            {
                for(const auto &supportedLang: *KP::supportedLangs) {
                    QSqlQuery query;
                    query.prepare("SELECT "+supportedLang+" FROM MapNode "
                                                          "WHERE MapID = :id;");
                    query.bindValue(":id", mapID);
                    if(!query.exec()) {
                        //% "Load map table failed!"
                        throw DBError(qtTrId("map-refresh-failed"),
                                      query.lastError(), query.lastQuery());
                        return false;
                    }
                    query.isSelect();
                    if(query.next()) {
                        m.localNames[supportedLang] = query.value(0).toString();
                    }
                }
            }
            if(mapID == KP::hiddenMap) {
                normalMaps.insert(
                    mapID + KP::Historical * KP::mapIDDifficultyMask,
                    new MapWithDiff(m, KP::Historical));
            }
            else {
                auto meta = QMetaEnum::fromType<KP::Difficulty>();
                for(int i = 0; i < meta.keyCount(); ++i) {
                    auto diff = static_cast<KP::Difficulty>(meta.value(i));
                    if(diff == KP::Historical) {
                        continue;
                    }
                    normalMaps.insert(
                        mapID + meta.value(i) * KP::mapIDDifficultyMask,
                        new MapWithDiff(m, diff));
                }
            }
        }
    }
    //% "Load map registry success!"
    qInfo() << qtTrId("map-load-good");

    return true;
}

void Server::generateEquipChilds(int originalChild, int thisEquip) {
    int fatherEquip = equipRegistry[thisEquip]->attr["Father"];
    int fatherEquip2 = equipRegistry[thisEquip]->attr["Father2"];
    int childEquip = originalChild;
    if(fatherEquip != 0) {
        equipChildTree.insert(fatherEquip, childEquip);
        generateEquipChilds(childEquip, fatherEquip);
    }
    if(fatherEquip2 != 0) {
        equipChildTree.insert(fatherEquip2, childEquip);
        generateEquipChilds(childEquip, fatherEquip2);
    }
}

QT_END_NAMESPACE
