/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#define NOMINMAX
#include "server.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>

#include "kerrors.h"

QT_BEGIN_NAMESPACE

namespace {

/* User registery */
Q_GLOBAL_STATIC(QString,
                userTable,
                QStringLiteral(
                    "CREATE TABLE NewUsers ( "
                    "UserID BLOB PRIMARY KEY, "
                    "UserType TEXT NOT NULL DEFAULT 'commoner'"
                    ");"
                    ))

/* User-bound attributes */
Q_GLOBAL_STATIC(QString,
                userAttr,
                QStringLiteral(
                    "CREATE TABLE UserAttr ( "
                    "UserID BLOB NOT NULL, "
                    "Attribute TEXT NOT NULL, "
                    "Intvalue INTEGER DEFAULT 0, "
                    "Realvalue REAL DEFAULT NULL, "
                    "FOREIGN KEY(UserID) REFERENCES NewUsers(UserID)"
                    "CONSTRAINT noduplicate UNIQUE(UserID, Attribute)"
                    ");"
                    ))

/* Equipment registry */
Q_GLOBAL_STATIC(QString,
                equipReg,
                QStringLiteral(
                    "CREATE TABLE EquipReg ( "
                    "EquipID INTEGER NOT NULL, "
                    "Attribute TEXT NOT NULL, "
                    "Intvalue INTEGER DEFAULT 0,"
                    "CONSTRAINT noduplicate UNIQUE(EquipID, Attribute)"
                    ");"
                    ))

/* Equipment name table */
Q_GLOBAL_STATIC(QString,
                equipName,
                QStringLiteral(
                    "CREATE TABLE EquipName ( "
                    "EquipID INTEGER PRIMARY KEY, "
                    "ja_JP TEXT, "
                    "zh_CN TEXT, "
                    "en_US TEXT"
                    ");"
                    ))

/* Ship registry */
Q_GLOBAL_STATIC(QString,
                shipReg,
                QStringLiteral(
                    "CREATE TABLE ShipReg ( "
                    "ShipID INTEGER NOT NULL, "
                    "Attribute TEXT NOT NULL, "
                    "Intvalue INTEGER DEFAULT 0,"
                    "CONSTRAINT noduplicate UNIQUE(ShipID, Attribute)"
                    ");"
                    ))

/* Ship name table */
Q_GLOBAL_STATIC(QString,
                shipName,
                QStringLiteral(
                    "CREATE TABLE ShipName ( "
                    "ShipID INTEGER, "
                    "lang TEXT, "
                    "textattr TEXT, "
                    "value TEXT "
                    ");"
                    ))

/* Factory slots of users */
Q_GLOBAL_STATIC(QString,
                userFactory,
                QStringLiteral(
                    "CREATE TABLE Factories ("
                    "UserID BLOB NOT NULL, "
                    "FactoryID INTEGER NOT NULL, "
                    "CurrentJob INTEGER DEFAULT 0, "
                    "StartTime INTEGER, "
                    "SuccessTime INTEGER, "
                    "Done BOOL DEFAULT false, "
                    "Success BOOL DEFAULT false, "
                    /* ship to remodel */
                    "PrevUuid TEXT, "
                    "FOREIGN KEY(UserID) REFERENCES NewUsers(UserID),"
                    "FOREIGN KEY(PrevUuid) REFERENCES UserShip(ShipUuid),"
                    "CONSTRAINT noduplicate UNIQUE(UserID, FactoryID)"
                    ");"
                    ))

/* Repair slots of users */
Q_GLOBAL_STATIC(QString,
                userDock,
                QStringLiteral(
                    "CREATE TABLE Docks ("
                    "UserID BLOB NOT NULL, "
                    "DockID INTEGER NOT NULL, "
                    /* ship to repair */
                    "Uuid TEXT, "
                    "StartHP INTEGER, " // actually currentHP
                    "CurrentHP INTEGER, " // not used
                    "MaxHP INTEGER, "
                    "StartTime INTEGER, "
                    "SuccessTime INTEGER, "
                    "FOREIGN KEY(UserID) REFERENCES NewUsers(UserID),"
                    "FOREIGN KEY(Uuid) REFERENCES UserShip(ShipUuid),"
                    "CONSTRAINT noduplicate UNIQUE(UserID, DockID)"
                    ");"
                    ))

/* Equipment of users */
Q_GLOBAL_STATIC(QString,
                userEquip,
                QStringLiteral(
                    "CREATE TABLE UserEquip ("
                    "User BLOB NOT NULL, "
                    "EquipUuid TEXT PRIMARY KEY, "
                    "EquipDef INTEGER NOT NULL, "
                    "Star INTEGER DEFAULT 0, "
                    "FOREIGN KEY(User) REFERENCES NewUsers(UserID), "
                    "FOREIGN KEY(EquipDef) REFERENCES EquipName(EquipID)"
                    ");"
                    ))

/* Equipment of users(KC) */
Q_GLOBAL_STATIC(QString,
                userKCEquip,
                QStringLiteral(
                    "CREATE TABLE UserKCEquip ("
                    "EquipUuid TEXT PRIMARY KEY, "
                    "EquipDef INTEGER NOT NULL, "
                    "Star INTEGER DEFAULT 0, "
                    "SkillPoints INTEGER DEFAULT 0, "
                    "FOREIGN KEY(EquipDef) REFERENCES EquipName(EquipID)"
                    ");"
                    ))

/* Equipment skill points */
Q_GLOBAL_STATIC(QString,
                userEquipSkillPoints,
                QStringLiteral(
                    "CREATE TABLE UserEquipSP ("
                    "User BLOB NOT NULL, "
                    "EquipDef INTEGER NOT NULL, "
                    "Intvalue INTEGER DEFAULT 0, "
                    "FOREIGN KEY(User) REFERENCES NewUsers(UserID),"
                    "FOREIGN KEY(EquipDef) REFERENCES EquipName(EquipID),"
                    "CONSTRAINT noduplicate UNIQUE(User, EquipDef) "
                    ");"
                    ))

/* Plane losses for abnormal exit recovery */
Q_GLOBAL_STATIC(QString,
                userPlaneLosses,
                QStringLiteral(
                    "CREATE TABLE UserPlaneLosses ("
                    "User BLOB NOT NULL, "
                    "ShipUuid TEXT NOT NULL, "
                    "Slot INTEGER NOT NULL, "  // 1-5 for slot index
                    "EquipDef INTEGER NOT NULL, "
                    "LossCount INTEGER DEFAULT 0, "
                    "RemainingCount INTEGER DEFAULT 0, "
                    "Timestamp INTEGER DEFAULT 0, "
                    "FOREIGN KEY(User) REFERENCES NewUsers(UserID), "
                    "FOREIGN KEY(ShipUuid) REFERENCES UserShip(ShipUuid), "
                    "FOREIGN KEY(EquipDef) REFERENCES EquipName(EquipID), "
                    "CONSTRAINT noduplicate UNIQUE(User, ShipUuid, Slot) "
                    ");"
                    ))

/* Map node table */
Q_GLOBAL_STATIC(QString,
                mapNode,
                QStringLiteral(
                    "CREATE TABLE MapNode ( "
                    "MapID INTEGER PRIMARY KEY, "
                    "ja_JP TEXT, "
                    "zh_CN TEXT, "
                    "en_US TEXT "
                    ");"
                    ))

/* Map relation table */
Q_GLOBAL_STATIC(QString,
                mapRelation,
                QStringLiteral(
                    "CREATE TABLE MapRelation ( "
                    "Type TEXT, "
                    "Node1 INTEGER NOT NULL, "
                    "Node2 INTEGER NOT NULL, "
                    "FOREIGN KEY(Node1) REFERENCES MapNode(MapID),"
                    "FOREIGN KEY(Node2) REFERENCES MapNode(MapID) "
                    ");"
                    ))

/* Map resources table */
Q_GLOBAL_STATIC(QString,
                mapResource,
                QStringLiteral(
                    "CREATE TABLE MapResource ( "
                    "MapID INTEGER, "
                    "Attribute TEXT NOT NULL, "
                    "Intvalue INTEGER, "
                    "FOREIGN KEY(MapID) REFERENCES MapNode(MapID) "
                    "CONSTRAINT noduplicate UNIQUE(MapID, Attribute) "
                    ");"
                    ))

/* Ship of users */
Q_GLOBAL_STATIC(QString,
                userShip,
                QStringLiteral(
                    "CREATE TABLE UserShip ("
                    "User BLOB NOT NULL, "
                    "ShipUuid TEXT PRIMARY KEY, "
                    "ShipDef INTEGER NOT NULL, "
                    "Star INTEGER DEFAULT 0, "
                    "CurrentHP INTEGER DEFAULT 1, "
                    "Condition INTEGER DEFAULT 480, " // KP::conditionMax
                    "CondRecovTime INTEGER, "
                    "Exp INTEGER DEFAULT 0, "
                    // does not prevent gaining of more Exp but only its effects
                    "ExpCap INTEGER DEFAULT 0, "
                    "Slot1 TEXT, "
                    "Slot2 TEXT, "
                    "Slot3 TEXT, "
                    "Slot4 TEXT, "
                    "Slot5 TEXT, "
                    "SlotEX TEXT, "
                    "Slot1Planes INTEGER DEFAULT 0, "
                    "Slot2Planes INTEGER DEFAULT 0, "
                    "Slot3Planes INTEGER DEFAULT 0, "
                    "Slot4Planes INTEGER DEFAULT 0, "
                    "Slot5Planes INTEGER DEFAULT 0, "
                    "FleetIndex INTEGER DEFAULT -1, "
                    "FleetPosIndex INTEGER DEFAULT -1, "
                    "FleetFled INTEGER DEFAULT 0, "
                    "Fuel REAL DEFAULT 1.0, "
                    "Ammo REAL DEFAULT 1.0, "
                    "FOREIGN KEY(User) REFERENCES NewUsers(UserID), "
                    "FOREIGN KEY(ShipDef) REFERENCES ShipName(ShipID), "
                    "FOREIGN KEY(Slot1) REFERENCES UserEquip(EquipUuid), "
                    "FOREIGN KEY(Slot2) REFERENCES UserEquip(EquipUuid), "
                    "FOREIGN KEY(Slot3) REFERENCES UserEquip(EquipUuid), "
                    "FOREIGN KEY(Slot4) REFERENCES UserEquip(EquipUuid), "
                    "FOREIGN KEY(Slot5) REFERENCES UserEquip(EquipUuid), "
                    "FOREIGN KEY(SlotEX) REFERENCES UserEquip(EquipUuid) "
                    ");"
                    ))

/* Equipment of users(KC) */
Q_GLOBAL_STATIC(QString,
                userKCShip,
                QStringLiteral(
                    "CREATE TABLE UserKCShip ("
                    "ShipUuid TEXT PRIMARY KEY, "
                    "ShipDef INTEGER NOT NULL, "
                    "Exp INTEGER DEFAULT 0, "
                    "FOREIGN KEY(ShipDef) REFERENCES ShipName(ShipID)"
                    ");"
                    ))

/* Ship of users */
Q_GLOBAL_STATIC(QString,
                userShipBP,
                QStringLiteral(
                    "CREATE TABLE UserShipBP ("
                    "User BLOB NOT NULL, "
                    "ShipDef INTEGER NOT NULL, "
                    "Amount INTEGER DEFAULT 0, "
                    "FOREIGN KEY(User) REFERENCES NewUsers(UserID), "
                    "FOREIGN KEY(ShipDef) REFERENCES ShipName(ShipID) "
                    "CONSTRAINT noduplicate UNIQUE(User, ShipDef) "
                    ");"
                    ))

/* Drop info of users */
Q_GLOBAL_STATIC(QString,
                userShipDrop,
                QStringLiteral(
                    "CREATE TABLE UserShipDrop ("
                    "User BLOB NOT NULL, "
                    "ShipDef INTEGER NOT NULL, "
                    "Amount FLOAT, "
                    "FOREIGN KEY(User) REFERENCES NewUsers(UserID), "
                    "FOREIGN KEY(ShipDef) REFERENCES ShipName(ShipID) "
                    "CONSTRAINT noduplicate UNIQUE(User, ShipDef) "
                    ");"
                    ))

/* Map state info of users */
Q_GLOBAL_STATIC(QString,
                userMapState,
                QStringLiteral(
                    "CREATE TABLE UserMapState ("
                    "User BLOB NOT NULL, "
                    "MapDef INTEGER NOT NULL, "
                    "Supremacy FLOAT NOT NULL DEFAULT -1, "
                    /* gauge remaining can be negative which makes DLC maps
                     * easier, you still need to defeat boss flagship
                     * to win */
                    "GaugeC INTEGER NOT NULL DEFAULT 0, "
                    "GaugeB INTEGER NOT NULL DEFAULT 0, "
                    "GaugeA INTEGER NOT NULL DEFAULT 0, "
                    "GaugeH INTEGER NOT NULL DEFAULT 0, "
                    /* unlock stages */
                    /* use another table for user-map-puzzle-state */
                    "CState INTEGER NOT NULL DEFAULT 0, "
                    "BState INTEGER NOT NULL DEFAULT 0, "
                    "AState INTEGER NOT NULL DEFAULT 0, "
                    "HState INTEGER NOT NULL DEFAULT 0, "
                    "FOREIGN KEY(User) REFERENCES NewUsers(UserID), "
                    "FOREIGN KEY(MapDef) REFERENCES MapNode(MapID) "
                    "CONSTRAINT noduplicate UNIQUE(User, MapDef) "
                    ");"
                    ))

/* Ranking info of users */
Q_GLOBAL_STATIC(QString,
                userRanking,
                QStringLiteral(
                    "CREATE TABLE UserRanking ("
                    "User BLOB PRIMARY KEY, "
                    "CurrentVP FLOAT NOT NULL DEFAULT 0, " // Victory points
                    "PreviousVP FLOAT NOT NULL DEFAULT 0, "
                    "Industrial FLOAT NOT NULL DEFAULT 0, "
                    "FOREIGN KEY(User) REFERENCES NewUsers(UserID) "
                    ");"
                    ))

/* Precondition relation */
Q_GLOBAL_STATIC(QString,
                virtualCondRelation,
                QStringLiteral(
                    "CREATE TABLE VirtualCondRelation ( "
                    "EquipDef INTEGER NOT NULL, "
                    "MapDef INTEGER NOT NULL, "
                    "MinDiff INTEGER DEFAULT 1, " // 1 early 2 mid 3 late
                    "Factor FLOAT DEFAULT 1, " // 1 early 2 mid 3 late
                    "FOREIGN KEY(EquipDef) REFERENCES EquipName(EquipID), "
                    "FOREIGN KEY(MapDef) REFERENCES MapNode(MapID) "
                    ");"
                    ))

/* ARD purchase order log for refund clawback */
Q_GLOBAL_STATIC(QString,
                ardOrders,
                QStringLiteral(
                    "CREATE TABLE ARDOrders ( "
                    "OrderID INTEGER PRIMARY KEY, "
                    "UserID BLOB NOT NULL, "
                    "Units INTEGER NOT NULL, "
                    "Status TEXT NOT NULL DEFAULT 'active', "
                    "FOREIGN KEY(UserID) REFERENCES NewUsers(UserID) "
                    ");"
                    ))

} // namespace

void Server::sqlinit() const {
    /* User QSqlDatabase db = QSqlDatabase::database();
     * to access database in elsewhere */
    QSqlDatabase db =
            QSqlDatabase::addDatabase(
                settings->value("sql/driver", "QSQLITE").toString());
    /* Use SQLite for current testing */
    db.setHostName(settings->value("sql/hostname",
                                   "SpearofTanaka").toString());
    db.setDatabaseName(settings->value("sql/dbname", "ocean.db").toString());
    db.setUserName(settings->value("sql/adminname", "admin").toString());
    /* obviously, a different password in settings is recommended */
    db.setPassword(settings->value("sql/adminpw", "10000826").toString());
    bool ok = db.open();
    if(!ok) {
        //% "Open database failed!"
        throw DBError(qtTrId("open-db-failed"));
    }
    else {
        //% "SQL connection successful!"
        qInfo() << qtTrId("sql-connect-success");
        /* Database integrity check, the structure is defined here */
        QStringList tables = db.tables(QSql::Tables);
        if(!tables.contains("NewUsers")) {
            sqlinitUsers();
        }
        if(!tables.contains("UserAttr")) {
            sqlinitUserA();
        }
        else {
            /* Migration: add Realvalue column if absent (added for Sanity) */
            QSqlQuery pragma;
            pragma.exec("PRAGMA table_info(UserAttr)");
            bool hasRealvalue = false;
            while(pragma.next()) {
                if(pragma.value(1).toString() == "Realvalue") {
                    hasRealvalue = true;
                    break;
                }
            }
            if(!hasRealvalue) {
                QSqlQuery alter;
                alter.prepare(
                    "ALTER TABLE UserAttr ADD COLUMN Realvalue REAL DEFAULT NULL");
                if(!alter.exec()) {
                    //% "Failed to migrate UserAttr: add Realvalue column."
                    throw DBError(qtTrId("userattr-migrate-realvalue-failed"),
                                  alter.lastError(), alter.lastQuery());
                }
            }
        }
        if(!tables.contains("EquipReg")) {
            sqlinitEquip();
        }
        if(!tables.contains("EquipName")) {
            sqlinitEquipName();
        }
        if(!tables.contains("ShipReg")) {
            sqlinitShip();
        }
        if(!tables.contains("ShipName")) {
            sqlinitShipName();
        }
        if(!tables.contains("Factories")) {
            sqlinitFacto();
        }
        if(!tables.contains("Docks")) {
            sqlinitDock();
        }
        if(!tables.contains("UserEquip")) {
            sqlinitEquipU();
        }
        if(!tables.contains("UserEquipSP")) {
            sqlinitEquipSP();
        }
        if(!tables.contains("UserPlaneLosses")) {
            //% "User plane losses database does not exist, creating..."
            qWarning() << qtTrId("plane-losses-db-lack");
            QSqlQuery query;
            query.prepare(*userPlaneLosses);
            if(!query.exec()) {
                //% "User plane losses table creation failure."
                throw DBError(qtTrId("plane-losses-db-gen-failure"),
                              query.lastError(), query.lastQuery());
            }
        }
        if(!tables.contains("MapNode")) {
            sqlinitMapNode();
        }
        if(!tables.contains("MapRelation")) {
            sqlinitMapRelation();
        }
        if(!tables.contains("MapResource")) {
            sqlinitMapResource();
        }
        if(!tables.contains("UserShip")) {
            sqlinitShipU();
        }
        if(!tables.contains("UserKCEquip")) {
            sqlinitEquipUKC();
        }
        if(!tables.contains("UserKCShip")) {
            sqlinitShipUKC();
        }
        if(!tables.contains("UserShipBP")) {
            sqlinitShipUBP();
        }
        if(!tables.contains("UserShipDrop")) {
            sqlinitShipDrop();
        }
        if(!tables.contains("UserMapState")) {
            sqlinitUserM();
        }
        if(!tables.contains("UserRanking")) {
            sqlinitRank();
        }
        if(!tables.contains("VirtualCondRelation")) {
            sqlinitVCR();
        }
        if(!tables.contains("ARDOrders")) {
            sqlinitARDOrders();
        }
    }
}

void Server::sqlinitDock() const {
    //% "Dock database does not exist, creating..."
    qWarning() << qtTrId("dock-db-lack");
    QSqlQuery query;
    query.prepare(*userDock);
    if(!query.exec()) {
        //% "Create Dock database failed."
        throw DBError(qtTrId("dock-db-gen-failure"),
                      query.lastError(), query.lastQuery());
    }
}

void Server::sqlinitEquip() const {
    //% "Equipment database does not exist, creating..."
    qWarning() << qtTrId("equip-db-lack");
    QSqlQuery query;
    query.prepare(*equipReg);
    if(!query.exec()) {
        //% "Create Equipment database failed."
        throw DBError(qtTrId("equip-db-gen-failure"),
                      query.lastError(), query.lastQuery());
    }
}

void Server::sqlinitEquipName() const {
    //% "Equipment name database does not exist, creating..."
    qWarning() << qtTrId("equip-name-db-lack");
    QSqlQuery query;
    query.prepare(*equipName);
    if(!query.exec()) {
        //% "Create Equipment name database failed."
        throw DBError(qtTrId("equip-name-db-gen-failure"),
                      query.lastError(), query.lastQuery());
    }
}

void Server::sqlinitEquipSP() const {
    //% "User equipment skillpoints database does not exist, creating..."
    qWarning() << qtTrId("equip-sp-db-lack");
    QSqlQuery query;
    query.prepare(*userEquipSkillPoints);
    if(!query.exec()) {
        //% "User equipment skillpoints fetch failure."
        throw DBError(qtTrId("equip-sp-db-gen-failure"),
                      query.lastError(), query.lastQuery());
    }
}

void Server::sqlinitEquipU() const {
    //% "Equipment database for user does not exist, creating..."
    qWarning() << qtTrId("equip-db-user-lack");
    QSqlQuery query;
    query.prepare(*userEquip);
    if(!query.exec()) {
        //% "Create Equipment database for user failed."
        throw DBError(qtTrId("equip-db-user-gen-failure"),
                      query.lastError(), query.lastQuery());
    }
}

void Server::sqlinitEquipUKC() const {
    //% "Equipment database (kancolle) for user does not exist, creating..."
    qWarning() << qtTrId("equip-db-kc-user-lack");
    QSqlQuery query;
    query.prepare(*userKCEquip);
    if(!query.exec()) {
        //% "Create Equipment database (kancolle) for user failed."
        throw DBError(qtTrId("equip-db-kc-user-gen-failure"),
                      query.lastError(), query.lastQuery());
    }
}

void Server::sqlinitFacto() const {
    //% "Factory database does not exist, creating..."
    qWarning() << qtTrId("facto-db-lack");
    QSqlQuery query;
    query.prepare(*userFactory);
    if(!query.exec()) {
        //% "Create Factory database failed."
        throw DBError(qtTrId("facto-db-gen-failure"),
                      query.lastError(), query.lastQuery());
    }
}

void Server::sqlinitMapNode() const {
    //% "Map node database does not exist, creating..."
    qWarning() << qtTrId("map-node-db-lack");
    QSqlQuery query;
    query.prepare(*mapNode);
    if(!query.exec()) {
        //% "Create Map node database failed."
        throw DBError(qtTrId("map-node-db-gen-failure"),
                      query.lastError(), query.lastQuery());
    }
}

void Server::sqlinitMapRelation() const {
    //% "Map relation database does not exist, creating..."
    qWarning() << qtTrId("map-relation-db-lack");
    QSqlQuery query;
    query.prepare(*mapRelation);
    if(!query.exec()) {
        //% "Create Map relation database failed."
        throw DBError(qtTrId("map-relation-db-gen-failure"),
                      query.lastError(), query.lastQuery());
    }
}

void Server::sqlinitMapResource() const {
    //% "Map resource database does not exist, creating..."
    qWarning() << qtTrId("map-resource-db-lack");
    QSqlQuery query;
    query.prepare(*mapResource);
    if(!query.exec()) {
        //% "Create Map resource database failed."
        throw DBError(qtTrId("map-resource-db-gen-failure"),
                      query.lastError(), query.lastQuery());
    }
}

void Server::sqlinitRank() const {
    //% "Ranking database does not exist, creating..."
    qWarning() << qtTrId("ranking-lack");
    QSqlQuery query;
    query.prepare(*userRanking);
    if(!query.exec()) {
        //% "Create Ranking database failed."
        throw DBError(qtTrId("ranking-db-gen-failure"),
                      query.lastError(), query.lastQuery());
    }
}

void Server::sqlinitShip() const {
    //% "Ship database does not exist, creating..."
    qWarning() << qtTrId("ship-db-lack");
    QSqlQuery query;
    query.prepare(*shipReg);
    if(!query.exec()) {
        //% "Create Ship database failed."
        throw DBError(qtTrId("equip-ship-db-gen-failure"),
                      query.lastError(), query.lastQuery());
    }
}

void Server::sqlinitShipDrop() const {
    //% "Ship drop database does not exist, creating..."
    qWarning() << qtTrId("ship-db-drop-lack");
    QSqlQuery query;
    query.prepare(*userShipDrop);
    if(!query.exec()) {
        //% "Create Ship drop database failed."
        throw DBError(qtTrId("equip-ship-drop-db-gen-failure"),
                      query.lastError(), query.lastQuery());
    }
}

void Server::sqlinitShipName() const {
    //% "Ship name database does not exist, creating..."
    qWarning() << qtTrId("ship-name-db-lack");
    QSqlQuery query;
    query.prepare(*shipName);
    if(!query.exec()) {
        //% "Create Ship name failed."
        throw DBError(qtTrId("equip-ship-name-gen-failure"),
                      query.lastError(), query.lastQuery());
    }
}

void Server::sqlinitShipU() const {
    //% "Ship database for user does not exist, creating..."
    qWarning() << qtTrId("ship-db-user-lack");
    QSqlQuery query;
    query.prepare(*userShip);
    if(!query.exec()) {
        //% "Create Ship database for user failed."
        throw DBError(qtTrId("ship-db-user-gen-failure"),
                      query.lastError(), query.lastQuery());
    }
}

void Server::sqlinitShipUBP() const {
    //% "Ship blueprint database for user does not exist, creating..."
    qWarning() << qtTrId("ship-db-bp-user-lack");
    QSqlQuery query;
    query.prepare(*userShipBP);
    if(!query.exec()) {
        //% "Create Ship blueprint database for user failed."
        throw DBError(qtTrId("ship-db-bp-user-gen-failure"),
                      query.lastError(), query.lastQuery());
    }
}

void Server::sqlinitShipUKC() const {
    //% "Ship database (kancolle) for user does not exist, creating..."
    qWarning() << qtTrId("ship-db-kc-user-lack");
    QSqlQuery query;
    query.prepare(*userKCShip);
    if(!query.exec()) {
        //% "Create Ship database (kancolle) for user failed."
        throw DBError(qtTrId("ship-db-kc-user-gen-failure"),
                      query.lastError(), query.lastQuery());
    }
}

void Server::sqlinitUsers() const {
    //% "User database does not exist, creating..."
    qWarning() << qtTrId("user-db-lack");
    QSqlQuery query;
    query.prepare(*userTable);
    if(!query.exec()) {
        //% "Create User database failed."
        throw DBError(qtTrId("user-db-gen-failure"),
                      query.lastError(), query.lastQuery());
    }
}

void Server::sqlinitUserA() const {
    //% "User attributes database does not exist, creating..."
    qWarning() << qtTrId("user-db-attr-lack");
    QSqlQuery query;
    query.prepare(*userAttr);
    if(!query.exec()) {
        //% "Create User attributes database failed."
        throw DBError(qtTrId("user-db-attr-gen-failure"),
                      query.lastError(), query.lastQuery());
    }
}

void Server::sqlinitUserM() const {
    //% "User map info database does not exist, creating..."
    qWarning() << qtTrId("user-db-map-lack");
    QSqlQuery query;
    query.prepare(*userMapState);
    if(!query.exec()) {
        //% "Create User map info database failed."
        throw DBError(qtTrId("user-db-map-gen-failure"),
                      query.lastError(), query.lastQuery());
    }
}

void Server::sqlinitARDOrders() const {
    //% "ARD orders database does not exist, creating..."
    qWarning() << qtTrId("ard-orders-db-lack");
    QSqlQuery query;
    query.prepare(*ardOrders);
    if(!query.exec()) {
        //% "Create ARD orders database failed."
        throw DBError(qtTrId("ard-orders-db-gen-failure"),
                      query.lastError(), query.lastQuery());
    }
}

void Server::sqlinitVCR() const {
    //% "Virtual condition-map relation database does not exist, creating..."
    qWarning() << qtTrId("user-db-vcr-lack");
    QSqlQuery query;
    query.prepare(*virtualCondRelation);
    if(!query.exec()) {
        //% "Create virtual condition-map relation info database failed."
        throw DBError(qtTrId("user-db-vcr-gen-failure"),
                      query.lastError(), query.lastQuery());
    }
}

QT_END_NAMESPACE
