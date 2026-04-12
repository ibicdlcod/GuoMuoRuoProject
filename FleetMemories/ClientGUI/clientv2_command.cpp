/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "clientv2.h"

#include <QSettings>

#include "../steam/isteamfriends.h"
#include "../Protocol/commandline.h"
#include "../Protocol/utility.h"
#include "networkerror.h"
#include "steamauth.h"

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

    auto meta = QMetaEnum::fromType<KP::CommandType>();
    QString primaryNonConst = primary;
    Utility::titleCase(primaryNonConst);

    switch(meta.keyToValue(primaryNonConst.toUtf8())) {
    case KP::Switch:
        doSwitch(cmdParts);
        return true;
    case KP::Develop:
        if(gameState != KP::Factory) {
            return false;
        }
        else {
            doDevelop(cmdParts);
            return true;
        }
    case KP::Fetch:
        if(gameState != KP::Factory) {
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
    result.append({"disconnect",
        "connect",
        "register",
        "develop",
        "switch",
        "fetch"
    });
    result.sort(Qt::CaseInsensitive);
    return result;
}

/* Relic of CLI ui */
const QStringList Client::getValidCommands() const {
    QStringList result = QStringList();
    result.append(getCommands());
    if(socket.isEncrypted())
    {
        result.append("disconnect");
        result.append("switch");
        if(gameState == KP::Factory)
        {
            result.append("develop");
            result.append("fetch");
        }
    }
    else if(!attemptMode)
        result.append({"connect", "register"});
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
