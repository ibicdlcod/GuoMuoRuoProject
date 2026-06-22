/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "clientv2.h"

#include <QApplication>
#include <QFile>
#include <QJsonDocument>
#include <QSettings>

#include "../steam/isteamfriends.h"
#include "../Protocol/commandline.h"
#include "../Protocol/utility.h"
#include "networkerror.h"
#include "steamauth.h"
#include "ui/fleet/fleetview.h"
#include "ui/mainwindow.h"
#include "ui/sortie/sortie.h"

extern QFile *logFile;
extern std::unique_ptr<QSettings> settings;

/* Parse CLI commands */
bool Client::parse(const QString &input) {
    static QRegularExpression re("\\s+");
    QStringList cmdParts = input.split(re, Qt::SkipEmptyParts);
    if(cmdParts.length() > 0) {
        auto meta = QMetaEnum::fromType<KP::ConsoleCommandType>();
        QString primary = cmdParts[0];
        Utility::titleCase(primary);

        switch(meta.keyToValue(primary.toUtf8())) {
        case KP::Help:
            cmdParts.removeFirst();
            showHelp(cmdParts);
            displayPrompt();
            return true;
        case KP::Exit:
            exitGracefully();
            return true;
        case KP::Commands:
            showCommands(true);
            displayPrompt();
            return true;
        case KP::Allcommands:
            showCommands(false);
            displayPrompt();
            return true;
        default:
            /* Not consistently present commands */
            bool success = parseSpec(cmdParts);
            if(!success) {
                invalidCommand();
            }
            displayPrompt();
            return success;
        }
    }
    displayPrompt();
    return false;
}

/* Parse CLI commands, continued */
bool Client::parseSpec(const QStringList &cmdParts) {
    try {
        if(cmdParts.length() > 0) {

            auto meta = QMetaEnum::fromType<KP::ConsoleCommandType>();
            QString primary = cmdParts[0];
            Utility::titleCase(primary);

            switch(meta.keyToValue(primary.toUtf8())) {
            case KP::Connect:
                parseConnectReq(cmdParts);
                return true;
            case KP::Disconnect:
                parseDisconnectReq();
                return true;
            case KP::Switchcert:
                switchCert(cmdParts);
                return true;
            case KP::Messagetest:
                if(!loginCheck()) {
                    return false;
                }
                sendTestMessages();
                return true;
            default:
                if(!loggedIn()) {
                    //% "You are not online, command is invalid."
                    qWarning() << qtTrId("command-when-loggedout");
                    return false;
                }
                else {
                    return parseGameCommands(primary, cmdParts);
                }
                break;
            }
        }
        return false;
    } catch (NetworkError &e) {
        qWarning() << (clientName + ":") << e.what();
        return true;
    }
}

/* Parse CLI commands actually related to game */
bool Client::parseGameCommands(const QString &primary,
                                 const QStringList &cmdParts) {

    QString primaryLower = primary.toLower();

    /* New text-only commands that do not correspond to a network CommandType. */
    if(primaryLower == "homeport") {
        parseHomePortCommand(cmdParts);
        return true;
    }
    if(primaryLower == "list") {
        parseListCommands(cmdParts);
        return true;
    }
    if(primaryLower == "fleet") {
        parseFleetCommands(cmdParts);
        return true;
    }
    if(primaryLower == "sortie" || primaryLower == "node"
        || primaryLower == "battle") {
        parseSortieCommands(cmdParts);
        return true;
    }
    if(primaryLower == "expedition") {
        parseExpeditionCommands(cmdParts);
        return true;
    }
    if(primaryLower == "construct" || primaryLower == "clone") {
        parseConstructCommand(cmdParts);
        return true;
    }
    if(primaryLower == "arsenal") {
        parseArsenalCommands(cmdParts);
        return true;
    }
    if(primaryLower == "anchorage") {
        parseAnchorageCommands(cmdParts);
        return true;
    }
    if(primaryLower == "repair" || primaryLower == "dock") {
        parseRepairCommands(cmdParts);
        return true;
    }
    if(primaryLower == "buy") {
        parseBuyCommand(cmdParts);
        return true;
    }
    if(primaryLower == "tech") {
        parseTechCommand(cmdParts);
        return true;
    }
    if(primaryLower == "query") {
        parseQueryCommand(cmdParts);
        return true;
    }
    if(primaryLower == "chrono") {
        parseChronoCommand(cmdParts);
        return true;
    }

    auto meta = QMetaEnum::fromType<KP::CommandType>();
    QString primaryNonConst = primary;
    Utility::titleCase(primaryNonConst);

    switch(meta.keyToValue(primaryNonConst.toUtf8())) {
    case KP::Switch:
        doSwitch(cmdParts);
        return true;
    case KP::Develop:
        if(!aiMode && gameState != KP::Factory) {
            return false;
        }
        else {
            doDevelop(cmdParts);
            return true;
        }
    case KP::Fetch:
        if(!aiMode && gameState != KP::Factory) {
            return false;
        }
        else {
            doFetch(cmdParts);
            return true;
        }
    case KP::Adminaddequip:
        doAddEquip(cmdParts);
        return true;
    case KP::Admingenerateequips:
        doGenerateTestEquip();
        return true;
    case KP::Adminremoveequips:
        doDeleteTestEquip();
        return true;
    case KP::Admingenerateships:
        doGenerateTestShip();
        return true;
    case KP::Adminremoveships:
        doDeleteTestShip();
        return true;
    case KP::Refresh:
        if(cmdParts.length() > 1
            && cmdParts[1].compare("Factory", Qt::CaseInsensitive) == 0) {
            doRefreshFactory();
            return true;
        } else {
            return false;
        }
    default:
        return false;
    }
}

/* Locate the main window from the list of top-level widgets. */
MainWindow * Client::getMainWindow() const {
    for(QWidget *widget: QApplication::topLevelWidgets()) {
        MainWindow *mainWindow = qobject_cast<MainWindow *>(widget);
        if(mainWindow) {
            return mainWindow;
        }
    }
    return nullptr;
}

/* homeport <nation> */
void Client::parseHomePortCommand(const QStringList &cmdParts) {
    if(cmdParts.length() < 2) {
        //% "Usage: homeport <nation>"
        emit qout(qtTrId("homeport-usage"));
        return;
    }
    auto meta = QMetaEnum::fromType<KP::AllegianceGroup>();
    int value = meta.keyToValue(cmdParts[1].toLatin1().constData());
    if(value == -1) {
        //% "Invalid nation: %1"
        qWarning() << qtTrId("homeport-invalid-nation").arg(cmdParts[1]);
        return;
    }
    chooseHomePort(static_cast<KP::AllegianceGroup>(value));
}

/* list <subcommand> ... */
void Client::parseListCommands(const QStringList &cmdParts) {
    if(cmdParts.length() < 2) {
        //% "Usage: list <ships|equips|blueprints>"
        emit qout(qtTrId("list-usage"));
        return;
    }
    QString sub = cmdParts[1].toLower();
    if(sub == "ships") {
        QHash<QUuid, Ship *> ships = shipModel.getAllShips();
        if(ships.isEmpty()) {
            //% "No ships available."
            emit qout(qtTrId("list-ships-empty"));
            return;
        }
        for(auto it = ships.cbegin(); it != ships.cend(); ++it) {
            emit qout(it.key().toString());
        }
        return;
    }
    if(sub == "equips") {
        const QHash<QUuid, Equipment *> equips =
            equipModel.getClientEquips();
        if(equips.isEmpty()) {
            //% "No equipment available."
            emit qout(qtTrId("list-equips-empty"));
            return;
        }
        for(auto it = equips.cbegin(); it != equips.cend(); ++it) {
            emit qout(it.key().toString());
        }
        return;
    }
    if(sub == "blueprints") {
        const QHash<int, int> blueprints = shipBPModel.getClientShipBPs();
        if(blueprints.isEmpty()) {
            //% "No blueprints available."
            emit qout(qtTrId("list-blueprints-empty"));
            return;
        }
        for(auto it = blueprints.cbegin(); it != blueprints.cend(); ++it) {
            emit qout(QStringLiteral("%1 %2").arg(it.key()).arg(it.value()));
        }
        return;
    }
    //% "Unknown list target: %1"
    qWarning() << qtTrId("list-unknown-target").arg(sub);
}

/* fleet <subcommand> ... */
void Client::parseFleetCommands(const QStringList &cmdParts) {
    if(cmdParts.length() < 2) {
        //% "Usage: fleet <set|clear|type|equip|planes|save|supply> ..."
        emit qout(qtTrId("fleet-usage"));
        return;
    }
    if(aiMode) {
        parseFleetCommandsHeadless(cmdParts);
        return;
    }
    MainWindow *mainWindow = getMainWindow();
    if(!mainWindow) {
        //% "Main window not available."
        qWarning() << qtTrId("cli-no-mainwindow");
        return;
    }
    FleetView *fleetView = mainWindow->getFleetArea();
    if(!fleetView) {
        //% "Fleet view not available."
        qWarning() << qtTrId("cli-no-fleetview");
        return;
    }
    QString sub = cmdParts[1].toLower();
    if(sub == "set") {
        if(cmdParts.length() < 5) {
            //% "Usage: fleet set <fleetindex> <posindex> <ship-uuid>"
            emit qout(qtTrId("fleet-set-usage"));
            return;
        }
        int fleetIndex = cmdParts[2].toInt();
        int posIndex = cmdParts[3].toInt();
        QUuid shipUuid(cmdParts[4]);
        if(shipUuid.isNull()) {
            //% "Invalid ship UUID: %1"
            qWarning() << qtTrId("fleet-invalid-ship-uuid").arg(cmdParts[4]);
            return;
        }
        fleetView->cliSetFleetShip(fleetIndex, posIndex, shipUuid);
        return;
    }
    if(sub == "clear") {
        if(cmdParts.length() < 4) {
            //% "Usage: fleet clear <fleetindex> <posindex>"
            emit qout(qtTrId("fleet-clear-usage"));
            return;
        }
        int fleetIndex = cmdParts[2].toInt();
        int posIndex = cmdParts[3].toInt();
        fleetView->cliClearFleetShip(fleetIndex, posIndex);
        return;
    }
    if(sub == "type") {
        if(cmdParts.length() < 4) {
            //% "Usage: fleet type <fleetindex> <NormalFleet|CombinedFleet>"
            emit qout(qtTrId("fleet-type-usage"));
            return;
        }
        int fleetIndex = cmdParts[2].toInt();
        fleetView->cliSetFleetType(fleetIndex, cmdParts[3]);
        return;
    }
    if(sub == "equip") {
        if(cmdParts.length() < 6) {
            //% "Usage: fleet equip <fleetindex> <posindex> <slot> "
            //% "<equip-uuid|clear>"
            emit qout(qtTrId("fleet-equip-usage"));
            return;
        }
        int fleetIndex = cmdParts[2].toInt();
        int posIndex = cmdParts[3].toInt();
        int slot = cmdParts[4].toInt();
        fleetView->cliSetShipEquip(fleetIndex, posIndex, slot, cmdParts[5]);
        return;
    }
    if(sub == "planes") {
        if(cmdParts.length() < 6) {
            //% "Usage: fleet planes <fleetindex> <posindex> <slot> <count>"
            emit qout(qtTrId("fleet-planes-usage"));
            return;
        }
        int fleetIndex = cmdParts[2].toInt();
        int posIndex = cmdParts[3].toInt();
        int slot = cmdParts[4].toInt();
        int count = cmdParts[5].toInt();
        fleetView->cliSetPlaneCount(fleetIndex, posIndex, slot, count);
        return;
    }
    if(sub == "save") {
        fleetView->cliSaveFleet();
        return;
    }
    if(sub == "supply") {
        if(cmdParts.length() < 3) {
            //% "Usage: fleet supply <fleetindex>"
            emit qout(qtTrId("fleet-supply-usage"));
            return;
        }
        int fleetIndex = cmdParts[2].toInt();
        fleetView->cliSupplyFleet(fleetIndex);
        return;
    }
    //% "Unknown fleet subcommand: %1"
    qWarning() << qtTrId("fleet-unknown-subcommand").arg(sub);
}

/* Send one headless fleet entry to the server. */
void Client::sendHeadlessFleetEntry(int fleetPos) {
    if(!headlessFleetData.contains(fleetPos)) {
        return;
    }
    QJsonArray content;
    content.append(headlessFleetData[fleetPos]);
    sendFleetData(content);
}

/* Headless fleet command: edit and submit FleetData. */
void Client::parseFleetCommandsHeadless(const QStringList &cmdParts) {
    QString sub = cmdParts[1].toLower();
    if(sub == "supply") {
        if(cmdParts.length() < 3) {
            //% "Usage: fleet supply <fleetindex>"
            emit qout(qtTrId("fleet-supply-usage"));
            return;
        }
        int fleetIndex = cmdParts[2].toInt();
        QJsonArray shipsToSupply;
        for(int posIndex = 0; posIndex < KP::combinedFleetSize; ++posIndex) {
            int fleetPos = fleetIndex * KP::fleetRepSize + posIndex;
            if(!headlessFleetData.contains(fleetPos)) {
                continue;
            }
            QUuid shipUuid(headlessFleetData[fleetPos]["uuid"].toString());
            if(shipUuid.isNull()) {
                continue;
            }
            QJsonObject entry;
            entry["uuid"] = shipUuid.toString();
            entry["fuel"] = true;
            entry["ammo"] = true;
            shipsToSupply.append(entry);
        }
        if(shipsToSupply.isEmpty()) {
            //% "Fleet %1 is empty; nothing to supply."
            qWarning() << qtTrId("fleet-headless-supply-empty").arg(fleetIndex);
            return;
        }
        doSupplyShip(shipsToSupply);
        return;
    }
    if(sub == "equip" || sub == "planes") {
        if(cmdParts.length() < 6) {
            //% "Usage: fleet equip|planes <fleetindex> <posindex> <slot> "
            //% "<equip-uuid|clear|count>"
            emit qout(qtTrId("fleet-headless-equip-usage"));
            return;
        }
        int fleetIndex = cmdParts[2].toInt();
        int posIndex = cmdParts[3].toInt();
        int fleetPos = fleetIndex * KP::fleetRepSize + posIndex;
        if(!headlessFleetData.contains(fleetPos)) {
            //% "No ship at fleet %1 position %2."
            qWarning() << qtTrId("fleet-headless-no-ship")
                              .arg(fleetIndex).arg(posIndex);
            return;
        }
        int slot = cmdParts[4].toInt();
        QJsonObject ship = headlessFleetData[fleetPos];
        if(sub == "equip") {
            if(slot < 0 || slot > KP::maxEquipSlots) {
                //% "Equipment slot must be 0..%1"
                qWarning() << qtTrId("fleet-headless-bad-equip-slot")
                                  .arg(KP::maxEquipSlots);
                return;
            }
            QJsonArray equips = ship["equip"].toArray();
            while(equips.size() <= KP::maxEquipSlots) {
                equips.append(QUuid().toString());
            }
            QString value = cmdParts[5].toLower();
            if(value == "clear") {
                equips[slot] = QUuid().toString();
            }
            else {
                QUuid equipUuid(value);
                if(equipUuid.isNull()) {
                    //% "Invalid equipment UUID: %1"
                    qWarning() << qtTrId("fleet-invalid-equip-uuid")
                                      .arg(cmdParts[5]);
                    return;
                }
                equips[slot] = equipUuid.toString();
            }
            ship["equip"] = equips;
        }
        else {
            if(slot < 0 || slot >= KP::maxEquipSlots) {
                //% "Plane slot must be 0..%1"
                qWarning() << qtTrId("fleet-headless-bad-plane-slot")
                                  .arg(KP::maxEquipSlots - 1);
                return;
            }
            QJsonArray planes = ship["plane"].toArray();
            while(planes.size() < KP::maxEquipSlots) {
                planes.append(0);
            }
            planes[slot] = cmdParts[5].toInt();
            ship["plane"] = planes;
        }
        headlessFleetData[fleetPos] = ship;
        sendHeadlessFleetEntry(fleetPos);
        return;
    }
    if(sub != "set" && sub != "clear") {
        //% "Headless fleet only supports set/clear/supply/equip/planes."
        qWarning() << qtTrId("fleet-headless-unsupported");
        return;
    }
    if(cmdParts.length() < 4) {
        //% "Usage: fleet set|clear <fleetindex> <posindex> [ship-uuid]"
        emit qout(qtTrId("fleet-headless-usage"));
        return;
    }
    int fleetIndex = cmdParts[2].toInt();
    int posIndex = cmdParts[3].toInt();
    int fleetPos = fleetIndex * KP::fleetRepSize + posIndex;
    if(sub == "set") {
        if(cmdParts.length() < 5) {
            emit qout(qtTrId("fleet-headless-usage"));
            return;
        }
        QUuid shipUuid(cmdParts[4]);
        if(shipUuid.isNull()) {
            //% "Invalid ship UUID: %1"
            qWarning() << qtTrId("fleet-invalid-ship-uuid").arg(cmdParts[4]);
            return;
        }
        QJsonObject ship;
        ship["uuid"] = shipUuid.toString();
        ship["pos"] = fleetPos;
        ship["fleettype"] = 0;
        QJsonArray equips;
        for(int i = 0; i <= KP::maxEquipSlots; ++i) {
            equips.append(QUuid().toString());
        }
        ship["equip"] = equips;
        QJsonArray planes;
        for(int i = 0; i < KP::maxEquipSlots; ++i) {
            planes.append(0);
        }
        ship["plane"] = planes;
        headlessFleetData.insert(fleetPos, ship);
        sendHeadlessFleetEntry(fleetPos);
    }
    else {
        headlessFleetData.remove(fleetPos);
        QJsonArray content;
        QJsonObject ship;
        ship["uuid"] = QUuid().toString();
        ship["pos"] = fleetPos;
        ship["fleettype"] = 0;
        QJsonArray equips;
        for(int i = 0; i <= KP::maxEquipSlots; ++i) {
            equips.append(QUuid().toString());
        }
        ship["equip"] = equips;
        QJsonArray planes;
        for(int i = 0; i < KP::maxEquipSlots; ++i) {
            planes.append(0);
        }
        ship["plane"] = planes;
        content.append(ship);
        sendFleetData(content);
    }
}

/* sortie / node / battle commands */
void Client::parseSortieCommands(const QStringList &cmdParts) {
    if(aiMode) {
        parseSortieCommandsHeadless(cmdParts);
        return;
    }
    MainWindow *mainWindow = getMainWindow();
    if(!mainWindow) {
        //% "Main window not available."
        qWarning() << qtTrId("cli-no-mainwindow");
        return;
    }
    QLayout *layout = mainWindow->getFleetAreaWidget();
    Sortie *sortie = nullptr;
    for(int i = 0; i < layout->count(); ++i) {
        sortie = qobject_cast<Sortie *>(layout->itemAt(i)->widget());
        if(sortie) {
            break;
        }
    }
    if(!sortie) {
        //% "Sortie view not available."
        qWarning() << qtTrId("cli-no-sortie");
        return;
    }
    QString primary = cmdParts[0].toLower();
    if(primary == "sortie") {
        if(cmdParts.length() < 2) {
            //% "Usage: sortie <mapid> <fleetindex> | sortie retreat | "
            //% "sortie advance"
            emit qout(qtTrId("sortie-usage"));
            return;
        }
        QString arg1 = cmdParts[1].toLower();
        if(arg1 == "retreat") {
            sortie->cliSortieRetreat();
            return;
        }
        if(arg1 == "advance") {
            sortie->cliSortieAdvance();
            return;
        }
        if(cmdParts.length() < 3) {
            //% "Usage: sortie <mapid> <fleetindex>"
            emit qout(qtTrId("sortie-map-usage"));
            return;
        }
        int mapId = cmdParts[1].toInt();
        int fleetIndex = cmdParts[2].toInt();
        sortie->cliSortie(mapId, fleetIndex);
        return;
    }
    if(primary == "node") {
        if(cmdParts.length() < 3 || cmdParts[1].toLower() != "choose") {
            //% "Usage: node choose <nodeid>"
            emit qout(qtTrId("node-choose-usage"));
            return;
        }
        int nodeId = cmdParts[2].toInt();
        sortie->cliChooseNode(nodeId);
        return;
    }
    if(primary == "battle") {
        if(cmdParts.length() < 3 || cmdParts[1].toLower() != "plan") {
            //% "Usage: battle plan <path-to-json>"
            emit qout(qtTrId("battle-plan-usage"));
            return;
        }
        QFile file(cmdParts[2]);
        if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            //% "Cannot open battle plan file: %1"
            qWarning() << qtTrId("battle-plan-file-error").arg(cmdParts[2]);
            return;
        }
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        if(!doc.isObject()) {
            //% "Battle plan file must contain a JSON object."
            qWarning() << qtTrId("battle-plan-invalid-json");
            return;
        }
        sortie->cliBattlePlan(doc.object());
        return;
    }
}

/* Headless sortie/node/battle: send raw builders. */
void Client::parseSortieCommandsHeadless(const QStringList &cmdParts) {
    QString primary = cmdParts[0].toLower();
    if(primary == "sortie") {
        if(cmdParts.length() < 2) {
            //% "Usage: sortie <mapid> <fleetindex> | sortie retreat | sortie advance"
            emit qout(qtTrId("sortie-usage"));
            return;
        }
        QString arg1 = cmdParts[1].toLower();
        if(arg1 == "retreat") {
            queryNextNode(currentMapId, currentNodeId, true);
            return;
        }
        if(arg1 == "advance") {
            queryNextNode(currentMapId, currentNodeId, false);
            return;
        }
        if(cmdParts.length() < 3) {
            //% "Usage: sortie <mapid> <fleetindex>"
            emit qout(qtTrId("sortie-map-usage"));
            return;
        }
        bool okMap = false;
        bool okFleet = false;
        int mapId = cmdParts[1].toInt(&okMap);
        int fleetIndex = cmdParts[2].toInt(&okFleet);
        if(!okMap || !okFleet) {
            emit qout(qtTrId("sortie-map-usage"));
            return;
        }
        currentMapId = mapId;
        currentNodeId = -1;
        lastSortieFleetIndex = fleetIndex;
        sortie(mapId, fleetIndex, false);
        return;
    }
    if(primary == "node") {
        if(cmdParts.length() < 3 || cmdParts[1].toLower() != "choose") {
            //% "Usage: node choose <nodeid>"
            emit qout(qtTrId("node-choose-usage"));
            return;
        }
        bool ok = false;
        int nodeId = cmdParts[2].toInt(&ok);
        if(!ok) {
            emit qout(qtTrId("node-choose-usage"));
            return;
        }
        chooseNode(currentMapId, nodeId);
        return;
    }
    if(primary == "battle") {
        if(cmdParts.length() < 3 || cmdParts[1].toLower() != "plan") {
            //% "Usage: battle plan <path-to-json>"
            emit qout(qtTrId("battle-plan-usage"));
            return;
        }
        QFile file(cmdParts[2]);
        if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            //% "Cannot open battle plan file: %1"
            qWarning() << qtTrId("battle-plan-file-error").arg(cmdParts[2]);
            return;
        }
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        if(!doc.isObject()) {
            //% "Battle plan file must contain a JSON object."
            qWarning() << qtTrId("battle-plan-invalid-json");
            return;
        }
        doBattle(doc.object());
        return;
    }
}

/* expedition <subcommand> ... */
void Client::parseExpeditionCommands(const QStringList &cmdParts) {
    MainWindow *mainWindow = getMainWindow();
    if(!mainWindow) {
        //% "Main window not available."
        qWarning() << qtTrId("cli-no-mainwindow");
        return;
    }
    QLayout *layout = mainWindow->getFleetAreaWidget();
    Sortie *sortie = nullptr;
    for(int i = 0; i < layout->count(); ++i) {
        sortie = qobject_cast<Sortie *>(layout->itemAt(i)->widget());
        if(sortie) {
            break;
        }
    }
    if(!sortie) {
        //% "Sortie view not available."
        qWarning() << qtTrId("cli-no-sortie");
        return;
    }
    if(aiMode) {
        //% "Expedition commands are not supported in headless AI mode."
        qWarning() << qtTrId("expedition-headless-unsupported");
        return;
    }
    if(cmdParts.length() < 2) {
        //% "Usage: expedition <start|cancel|settings|plan|plans> ..."
        emit qout(qtTrId("expedition-usage"));
        return;
    }
    QString sub = cmdParts[1].toLower();
    if(sub == "start") {
        if(cmdParts.length() < 4) {
            //% "Usage: expedition start <mapid> <fleetindex> "
            //% "[threshold] [autoresupply]"
            emit qout(qtTrId("expedition-start-usage"));
            return;
        }
        int mapId = cmdParts[2].toInt();
        int fleetIndex = cmdParts[3].toInt();
        double threshold = 1.0;
        bool autoResupply = true;
        if(cmdParts.length() > 4) {
            threshold = cmdParts[4].toDouble();
        }
        if(cmdParts.length() > 5) {
            autoResupply = (cmdParts[5].toLower() == "true"
                            || cmdParts[5].toLower() == "1"
                            || cmdParts[5].toLower() == "yes");
        }
        sortie->cliExpeditionStart(mapId, fleetIndex, threshold,
                                   autoResupply);
        return;
    }
    if(sub == "cancel") {
        if(cmdParts.length() < 4) {
            //% "Usage: expedition cancel <mapid> <fleetindex>"
            emit qout(qtTrId("expedition-cancel-usage"));
            return;
        }
        int mapId = cmdParts[2].toInt();
        int fleetIndex = cmdParts[3].toInt();
        sortie->cliExpeditionCancel(mapId, fleetIndex);
        return;
    }
    if(sub == "settings") {
        if(cmdParts.length() < 5) {
            //% "Usage: expedition settings <mapid> <threshold> <autoresupply>"
            emit qout(qtTrId("expedition-settings-usage"));
            return;
        }
        int mapId = cmdParts[2].toInt();
        double threshold = cmdParts[3].toDouble();
        bool autoResupply = (cmdParts[4].toLower() == "true"
                             || cmdParts[4].toLower() == "1"
                             || cmdParts[4].toLower() == "yes");
        sortie->cliExpeditionSettings(mapId, threshold, autoResupply);
        return;
    }
    if(sub == "plan") {
        if(cmdParts.length() < 5) {
            //% "Usage: expedition plan <mapid> <nodeid> <path-to-plan>"
            emit qout(qtTrId("expedition-plan-usage"));
            return;
        }
        int mapId = cmdParts[2].toInt();
        int nodeId = cmdParts[3].toInt();
        QFile file(cmdParts[4]);
        if(!file.open(QIODevice::ReadOnly)) {
            //% "Cannot open expedition plan file: %1"
            qWarning() << qtTrId("expedition-plan-file-error")
                              .arg(cmdParts[4]);
            return;
        }
        sortie->cliExpeditionPlan(mapId, nodeId, file.readAll());
        return;
    }
    if(sub == "plans") {
        if(cmdParts.length() < 3 || cmdParts[2].toLower() != "save") {
            //% "Usage: expedition plans save <mapid>"
            emit qout(qtTrId("expedition-plans-save-usage"));
            return;
        }
        int mapId = cmdParts[3].toInt();
        sortie->cliExpeditionPlansSave(mapId);
        return;
    }
    //% "Unknown expedition subcommand: %1"
    qWarning() << qtTrId("expedition-unknown-subcommand").arg(sub);
}

/* construct <shipdef> <slot> [remodeluuid|none] [equipuuid]... */
/* clone <shipdef> <slot> */
void Client::parseConstructCommand(const QStringList &cmdParts) {
    if(cmdParts.length() < 3) {
        //% "Usage: construct <shipdef> <slot> [remodel-uuid|none] "
        //% "[equip-uuid]..."
        emit qout(qtTrId("construct-usage"));
        return;
    }
    int shipDef = cmdParts[1].toInt();
    if(shipDef == 0) {
        //% "Invalid ship definition ID."
        qWarning() << qtTrId("construct-invalid-shipdef");
        return;
    }
    int slot = cmdParts[2].toInt();
    QUuid shipToRemodel;
    int equipStart = 3;
    if(cmdParts.length() > 3) {
        QString arg = cmdParts[3];
        if(arg.compare("none", Qt::CaseInsensitive) == 0) {
            shipToRemodel = QUuid();
            equipStart = 4;
        }
        else {
            QUuid parsed(arg);
            if(!parsed.isNull()) {
                shipToRemodel = parsed;
                equipStart = 4;
            }
        }
    }
    QList<QUuid> defaultEquips;
    for(int i = equipStart; i < cmdParts.length(); ++i) {
        if(cmdParts[i].compare("none", Qt::CaseInsensitive) == 0) {
            defaultEquips.append(QUuid());
            continue;
        }
        QUuid equipUuid(cmdParts[i]);
        if(equipUuid.isNull()) {
            //% "Invalid equipment UUID: %1"
            qWarning() << qtTrId("construct-invalid-equip-uuid")
                              .arg(cmdParts[i]);
            return;
        }
        defaultEquips.append(equipUuid);
    }
    doConstructShip(shipDef, defaultEquips, shipToRemodel, slot);
}

/* arsenal <subcommand> ... */
void Client::parseArsenalCommands(const QStringList &cmdParts) {
    if(cmdParts.length() < 2) {
        //% "Usage: arsenal <refresh|destruct|improve> ..."
        emit qout(qtTrId("arsenal-usage"));
        return;
    }
    QString sub = cmdParts[1].toLower();
    if(sub == "refresh") {
        doRefreshFactoryArsenal();
        return;
    }
    if(sub == "destruct" || sub == "improve") {
        if(cmdParts.length() < 3) {
            //% "Usage: arsenal %1 <equip-uuid>..."
            emit qout(qtTrId("arsenal-uuid-usage").arg(sub));
            return;
        }
        QList<QUuid> uuids;
        for(int i = 2; i < cmdParts.length(); ++i) {
            QUuid uuid(cmdParts[i]);
            if(uuid.isNull()) {
                //% "Invalid equipment UUID: %1"
                qWarning() << qtTrId("arsenal-invalid-uuid")
                                  .arg(cmdParts[i]);
                return;
            }
            uuids.append(uuid);
        }
        if(sub == "destruct") {
            doDestructEquip(uuids);
        }
        else {
            doImproveEquip(uuids);
        }
        return;
    }
    //% "Unknown arsenal subcommand: %1"
    qWarning() << qtTrId("arsenal-unknown-subcommand").arg(sub);
}

/* anchorage <subcommand> ... */
void Client::parseAnchorageCommands(const QStringList &cmdParts) {
    if(cmdParts.length() < 2) {
        //% "Usage: anchorage <refresh|modernize|decorate|supply|supplyall> "
        //% "..."
        emit qout(qtTrId("anchorage-usage"));
        return;
    }
    QString sub = cmdParts[1].toLower();
    if(sub == "refresh") {
        doRefreshFactoryAnchorage();
        return;
    }
    if(sub == "supplyall") {
        shipModel.enactSupplyAll();
        return;
    }
    if(sub == "modernize" || sub == "decorate" || sub == "supply") {
        if(cmdParts.length() < 3) {
            //% "Usage: anchorage %1 <ship-uuid>..."
            emit qout(qtTrId("anchorage-uuid-usage").arg(sub));
            return;
        }
        QList<QUuid> uuids;
        for(int i = 2; i < cmdParts.length(); ++i) {
            QUuid uuid(cmdParts[i]);
            if(uuid.isNull()) {
                //% "Invalid ship UUID: %1"
                qWarning() << qtTrId("anchorage-invalid-uuid")
                                  .arg(cmdParts[i]);
                return;
            }
            uuids.append(uuid);
        }
        if(sub == "modernize") {
            doModernizeShip(uuids);
        }
        else if(sub == "decorate") {
            doDecorateShip(uuids);
        }
        else {
            QJsonArray shipsToSupply;
            for(const QUuid &uuid: std::as_const(uuids)) {
                QJsonObject entry;
                entry["uuid"] = uuid.toString();
                entry["fuel"] = true;
                entry["ammo"] = true;
                shipsToSupply.append(entry);
            }
            doSupplyShip(shipsToSupply);
        }
        return;
    }
    //% "Unknown anchorage subcommand: %1"
    qWarning() << qtTrId("anchorage-unknown-subcommand").arg(sub);
}

/* repair / dock commands */
void Client::parseRepairCommands(const QStringList &cmdParts) {
    QString primary = cmdParts[0].toLower();
    if(primary == "dock") {
        if(cmdParts.length() < 2 || cmdParts[1].toLower() != "refresh") {
            //% "Usage: dock refresh"
            emit qout(qtTrId("dock-refresh-usage"));
            return;
        }
        doRefreshDock();
        return;
    }
    if(cmdParts.length() < 2) {
        //% "Usage: repair <ship-uuid> <slot> | repair stop <slot> | "
        //% "repair force <slot>"
        emit qout(qtTrId("repair-usage"));
        return;
    }
    QString sub = cmdParts[1].toLower();
    if(sub == "stop") {
        if(cmdParts.length() < 3) {
            //% "Usage: repair stop <slot>"
            emit qout(qtTrId("repair-stop-usage"));
            return;
        }
        doStopRepair(cmdParts[2].toInt());
        return;
    }
    if(sub == "force") {
        if(cmdParts.length() < 3) {
            //% "Usage: repair force <slot>"
            emit qout(qtTrId("repair-force-usage"));
            return;
        }
        doForceRepair(cmdParts[2].toInt());
        return;
    }
    if(cmdParts.length() < 3) {
        //% "Usage: repair <ship-uuid> <slot>"
        emit qout(qtTrId("repair-ship-usage"));
        return;
    }
    QUuid shipUuid(cmdParts[1]);
    if(shipUuid.isNull()) {
        //% "Invalid ship UUID: %1"
        qWarning() << qtTrId("repair-invalid-ship-uuid").arg(cmdParts[1]);
        return;
    }
    doRepair(shipUuid, cmdParts[2].toInt());
}

/* buy <subcommand> ... */
void Client::parseBuyCommand(const QStringList &cmdParts) {
    if(cmdParts.length() < 2) {
        //% "Usage: buy <equip|medal|resources|ard> ..."
        emit qout(qtTrId("buy-usage"));
        return;
    }
    QString sub = cmdParts[1].toLower();
    if(sub == "equip") {
        if(cmdParts.length() < 3) {
            //% "Usage: buy equip <equipdef>"
            emit qout(qtTrId("buy-equip-usage"));
            return;
        }
        doBuyFromStore(cmdParts[2].toInt());
        return;
    }
    if(sub == "medal") {
        if(cmdParts.length() < 3) {
            //% "Usage: buy medal <amount>"
            emit qout(qtTrId("buy-medal-usage"));
            return;
        }
        doBuyMedal(cmdParts[2].toInt());
        return;
    }
    if(sub == "resources") {
        if(cmdParts.length() < 4) {
            //% "Usage: buy resources <O|E|S|R|A|W|C> <coupons>"
            emit qout(qtTrId("buy-resources-usage"));
            return;
        }
        doBuyOrdinaryResources(cmdParts[2], cmdParts[3].toInt());
        return;
    }
    if(sub == "ard") {
        if(cmdParts.length() < 3) {
            //% "Usage: buy ard <units>"
            emit qout(qtTrId("buy-ard-usage"));
            return;
        }
        initARDPurchase(cmdParts[2].toInt());
        return;
    }
    //% "Unknown buy subcommand: %1"
    qWarning() << qtTrId("buy-unknown-subcommand").arg(sub);
}

/* tech <subcommand> ... */
void Client::parseTechCommand(const QStringList &cmdParts) {
    if(cmdParts.length() < 2) {
        //% "Usage: tech <demand|skillpoints|convert|global> ..."
        emit qout(qtTrId("tech-usage"));
        return;
    }
    QString sub = cmdParts[1].toLower();
    if(sub == "demand" || sub == "skillpoints") {
        if(cmdParts.length() < 3) {
            //% "Usage: tech %1 <defid>"
            emit qout(qtTrId("tech-def-usage").arg(sub));
            return;
        }
        int def = cmdParts[2].toInt();
        if(sub == "demand") {
            sendInfo(KP::clientDemandTech(def));
        }
        else {
            sendInfo(KP::clientDemandSkillPoints(def));
        }
        return;
    }
    if(sub == "convert") {
        if(cmdParts.length() < 5) {
            //% "Usage: tech convert <src-def> <dst-def> <amount>"
            emit qout(qtTrId("tech-convert-usage"));
            return;
        }
        int src = cmdParts[2].toInt();
        int dst = cmdParts[3].toInt();
        qint64 amount = cmdParts[4].toLongLong();
        sendInfo(KP::clientConvertSkillPoints(src, dst, amount));
        return;
    }
    if(sub == "global") {
        switchToTech2();
        return;
    }
    //% "Unknown tech subcommand: %1"
    qWarning() << qtTrId("tech-unknown-subcommand").arg(sub);
}

/* query <target> */
void Client::parseQueryCommand(const QStringList &cmdParts) {
    if(cmdParts.length() < 2) {
        //% "Usage: query <resources|supremacy|expedition|rank|arsenal|"
        //% "anchorage>"
        emit qout(qtTrId("query-usage"));
        return;
    }
    QString target = cmdParts[1].toLower();
    if(target == "resources") {
        demandResourceGain();
        return;
    }
    if(target == "supremacy") {
        demandMapSupremacy();
        return;
    }
    if(target == "expedition") {
        queryExpeditionStatus();
        return;
    }
    if(target == "rank") {
        int rows = 20;
        std::optional<int> page;
        if(cmdParts.length() > 2) {
            rows = cmdParts[2].toInt();
        }
        if(cmdParts.length() > 3) {
            page = cmdParts[3].toInt();
        }
        doRefreshRank(rows, page);
        return;
    }
    if(target == "arsenal") {
        doRefreshFactoryArsenal();
        return;
    }
    if(target == "anchorage") {
        doRefreshFactoryAnchorage();
        return;
    }
    //% "Unknown query target: %1"
    qWarning() << qtTrId("query-unknown-target").arg(target);
}

/* Parse connection request */
void Client::parseConnectReq(const QStringList &cmdParts) {

    conf.addCaCertificates(settings->value("networkclient/pem",
                                           ":/harusoft.pem").toString());
    socket.setSslConfiguration(conf);
    if(socket.isEncrypted()) {
        //% "Already connected, disconnect first."
        qInfo() << qtTrId("connected-already");
        return;
    }
    else if(attemptMode) {
        //% "Do not attempt duplicate connections!"
        qWarning() << qtTrId("connect-duplicate");
        return;
    }
    retransmitTimes = 0;
    if(cmdParts.length() < 3) {
        //% "Usage: connect [ip] [port]"
        emit qout(qtTrId("connect-usage"));
        return;
    }
    else {
        /* Send App ticek to server */
        address = QHostAddress(cmdParts[1]);
        if(address.isNull()) {
            //% "IP isn't valid."
            qWarning() << qtTrId("ip-invalid");
            return;
        }
        port = QString(cmdParts[2]).toInt();
        if(port < 1024 || port > 49151) {
            //% "Port isn't valid, it must fall between 1024 and 49151"
            qWarning() << qtTrId("port-invalid");
            return;
        }
        if(aiMode) {
            attemptMode = true;
            clientName = aiName;
            headlessConnect();
            return;
        }
        attemptMode = true;

        QObject::connect(&sauth, &SteamAuth::eATFailed,
                         this, &Client::catbomb);
        sauth.RetrieveEncryptedAppTicket();
        SteamAPI_RunCallbacks();

        QTimer::singleShot(
            std::chrono::milliseconds(settings->value(
                "networkclient/autopasswordtime", 1000).toInt()),
            this, &Client::autoPassword);
        clientName = SteamFriends()->GetPersonaName();

        return;
    }
}

/* Parse disconnection request */
void Client::parseDisconnectReq() {
    if(!socket.isEncrypted()) {
        //% "You are not online."
        qInfo() << qtTrId("disconnect-when-offline");
    }
    else {
        QByteArray msg = KP::clientSteamLogout();
        const qint64 written = socket.write(msg);
        //% "Attempting to disconnect..."
        qInfo() << qtTrId("disconnect-attempt");
        if (written <= 0) {
            throw NetworkError(socket.errorString());
        }
    }
}

/* Parse quit */
void Client::parseQuit() {
    if(gameState != KP::Offline)
        parseDisconnectReq();
    authSent = false;
    exitGracefully();
}

/* Relic of CLI */
void Client::qls(const QStringList &input) {
    emit qout(input.join(" "));
}

/* Show help in command line */
void Client::showHelp(const QStringList &cmdParts) {
    if(cmdParts.isEmpty()) {
        //% "Use 'exit' to quit, 'help' to show help, "
        //% "'commands' to show available commands."
        emit qout(qtTrId("help-msg"));
    }
    else { /* this trick does not do things nicely */
        parse(cmdParts[0]);
    }
}

/* CLI */
void Client::showCommands(bool validOnly) {
    //% "Use 'exit' to quit."
    emit qout(qtTrId("exit-helper"));
    if(validOnly) {
        //% "Available commands:"
        emit qout(qtTrId("good-command"),
                  QColor("black"), QColor("lightgreen"));
        qls(getValidCommands());
    }
    else {
        //% "All commands:"
        emit qout(qtTrId("all-command"), QColor("black"), QColor("yellow"));
        qls(getCommandsSpec());
    }
}

/* Relic of CLI ui */
const QStringList Client::getCommandsSpec() const {
    QStringList result = QStringList();
    result.append(getCommands());
    result.append({"connect",
        "disconnect",
        "switchcert",
        "messagetest",
        "switch",
        "develop",
        "fetch",
        "refresh",
        "addequip",
        "admingenerateequips",
        "adminremoveequips",
        "admingenerateships",
        "adminremoveships",
        "homeport",
        "list",
        "list ships",
        "list equips",
        "list blueprints",
        "fleet",
        "fleet set",
        "fleet clear",
        "fleet type",
        "fleet equip",
        "fleet planes",
        "fleet save",
        "fleet supply",
        "sortie",
        "sortie retreat",
        "sortie advance",
        "node",
        "node choose",
        "battle",
        "battle plan",
        "expedition",
        "expedition start",
        "expedition cancel",
        "expedition settings",
        "expedition plan",
        "expedition plans save",
        "construct",
        "clone",
        "arsenal",
        "arsenal refresh",
        "arsenal destruct",
        "arsenal improve",
        "anchorage",
        "anchorage refresh",
        "anchorage modernize",
        "anchorage decorate",
        "anchorage supply",
        "anchorage supplyall",
        "repair",
        "repair stop",
        "repair force",
        "dock",
        "dock refresh",
        "buy",
        "buy equip",
        "buy medal",
        "buy resources",
        "buy ard",
        "chrono",
        "tech",
        "tech demand",
        "tech skillpoints",
        "tech convert",
        "tech global",
        "query",
        "query resources",
        "query supremacy",
        "query expedition",
        "query rank",
        "query arsenal",
        "query anchorage"
    });
    result.removeDuplicates();
    result.sort(Qt::CaseInsensitive);
    return result;
}

/* Relic of CLI ui */
const QStringList Client::getValidCommands() const {
    QStringList result = QStringList();
    result.append(getCommands());
    if(socket.isEncrypted()) {
        /* Always valid when online. */
        result.append("disconnect");
        result.append("switch");
        result.append("buy");
        result.append("tech");
        result.append("query");
        result.append("homeport");

        /* Context-sensitive by game state. Port is the hub and allows
         * entering any activity. */
        bool const inPort = (gameState == KP::Port);
        bool const inFactory = (gameState == KP::Factory);
        bool const inFleet = (gameState == KP::FleetView);
        bool const inSortie = (gameState == KP::SortieMapView
                               || gameState == KP::BattleMapView);
        bool const inRepair = (gameState == KP::RepairView);

        if(inPort || inFleet) {
            result.append("fleet");
        }
        if(inPort || inSortie) {
            result.append("sortie");
            result.append("node");
            result.append("battle");
            result.append("expedition");
        }
        if(inPort || inFactory) {
            result.append("construct");
            result.append("clone");
            result.append("arsenal");
            result.append("anchorage");
        }
        if(inFactory) {
            result.append("develop");
            result.append("fetch");
            result.append("refresh");
        }
        if(inPort || inRepair) {
            result.append("repair");
            result.append("dock");
        }
        if(inPort) {
            result.append("addequip");
            result.append("admingenerateequips");
            result.append("adminremoveequips");
            result.append("admingenerateships");
            result.append("adminremoveships");
            result.append("switchcert");
            result.append("messagetest");
        }
    }
    else if(!attemptMode) {
        result.append({"connect", "register"});
    }
    result.removeDuplicates();
    result.sort(Qt::CaseInsensitive);
    return result;
}

/* CLI */
const QStringList Client::getCommands() {
    return CommandLine::getCommands();
}

/* Command that are not supported */
inline void Client::invalidCommand() {
    //% "Invalid Command, use 'commands' for valid commands, "
    //% "'help' for help, 'exit' to exit."
    emit qout(qtTrId("invalid-command"));
}

/* Switch view */
void Client::doSwitch(const QStringList &cmdParts) {
    if(gameState == KP::BattleMapView) {
        //% "You can't leave battle without normal methods."
        qWarning() << qtTrId("gamestate-battle-leave");
        return;
    }
    if(cmdParts.length() < 2) {
        //% "Usage: switch [gamestate]"
        emit qout(qtTrId("switch-usage"));
        return;
    }
    else {
        QString secondary = cmdParts[1].first(1).toUpper()
                            + cmdParts[1].sliced(1).toLower();

        QMetaEnum info = QMetaEnum::fromType<KP::GameState>();
        int statevalue = info.keyToValue(secondary.toLatin1().constData());
        if(statevalue == -1) {
            //% "Nonexistent gamestate: %1"
            qWarning() << qtTrId("game-unexpected-state").arg(secondary);
        }
        else if(statevalue == KP::BattleMapView) {
            //% "You can't enter battle without normal methods."
            qWarning() << qtTrId("gamestate-battle");
        }
        else if(statevalue == KP::Offline) {
            //% "Use 'disconnect' for logout."
            qWarning() << qtTrId("gamestate-offline");
        }
        else {
            gameState = (KP::GameState)statevalue;
            emit gamestateChanged(gameState);
        }
        return;
    }
}

/* Exit */
void Client::exitGracefully() {
    exitGraceSpec();
    disconnect(timer, &QTimer::timeout, this, &Client::uiRefresh);
    //% "Goodbye."
    emit qout(qtTrId("goodbye-gui"), QColor("black"), QColor(64,255,64));
    if(logFile) {
        logFile->close();
    }
    emit aboutToQuit();
}

/* Exit */
void Client::exitGraceSpec() {
    if(socket.isEncrypted()) {
        parseDisconnectReq();
    }
    shutdown();
}
