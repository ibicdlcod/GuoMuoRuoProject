/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include <QCoreApplication>
#include <QLocale>
#include <QSqlDatabase>
#include <QTranslator>
#include "../steam/steam_gameserver.h"

#include "../Protocol/kp.h"
#include "kerrors.h"
#include "qconsolelistener.h"
#include "server.h"

#if defined(Q_OS_UNIX)
#include <netinet/in.h>
#endif

QFile *logFile;
std::unique_ptr<QSettings> settings;

namespace {
const int STEAM_ERROR = 1;
}

int main(int argc, char *argv[]) {
    std::set_terminate([]() {
        auto eptr = std::current_exception();
        if(eptr) {
            try { std::rethrow_exception(eptr); }
            catch(DBError &e) {
                for(const QString &s : e.whats())
                    qCritical() << s;
            }
            catch(std::exception &e) {
                qCritical() << "Uncaught exception:" << e.what();
            }
            catch(...) {
                qCritical() << "Uncaught unknown exception";
            }
        }
        std::abort();
    });

    /* Test battle mode
     * — see doc/worldview_and_mechanics/9.t1-testbattle.md
     * [Implemented in main.cpp#test-battle-mode]
     * [Implemented in Server::runTestBattle] */
    {
        QString luaPath;
        QString reportPath;
        int repeatCount = 1;
        bool isTestMode = false;
        for(int i = 1; i < argc; ++i) {
            QString arg = QString::fromLocal8Bit(argv[i]);
            if(arg == QStringLiteral("--testbattle") && i + 1 < argc) {
                luaPath = QString::fromLocal8Bit(argv[++i]);
                isTestMode = true;
            }
            else if(arg == QStringLiteral("--report") && i + 1 < argc) {
                reportPath = QString::fromLocal8Bit(argv[++i]);
            }
            else if(arg == QStringLiteral("--repeat") && i + 1 < argc) {
                repeatCount
                    = QString::fromLocal8Bit(argv[++i]).toInt();
            }
        }
        if(isTestMode) {
            if(luaPath.isEmpty()) {
                qCritical() << "--testbattle requires a lua file path";
                return 1;
            }
            if(repeatCount > 1 && reportPath.isEmpty()) {
                qCritical()
                    << "--report is required when --repeat is specified";
                return 1;
            }
            if(repeatCount < 1)
                repeatCount = 1;

            QT_USE_NAMESPACE
            Server server(argc, argv);
            server.setApplicationName("FleetMemories Server");
            server.setApplicationVersion("0.60.1");
            server.setOrganizationName("Harusame Software");
            server.setOrganizationDomain("fleetmemories.moe");
            settings = std::make_unique<QSettings>(new QSettings);
            settings->setValue("server/language", "en_US");
            KP::initLog(true);

            QTranslator translator;
            const QString baseName
                = QStringLiteral("FleetMemories_")
                  + QLocale(
                        settings->value("server/language",
                                        "en_US")
                            .toString())
                        .name();
            if(translator.load(
                   QStringLiteral(":/i18n/") + baseName))
                server.installTranslator(&translator);

            QSqlDatabase db = QSqlDatabase::addDatabase(
                QStringLiteral("QSQLITE"));
            db.setDatabaseName(QStringLiteral(":memory:"));
            if(!db.open()) {
                qCritical() << "Failed to open in-memory database";
                return 1;
            }

            server.runTestBattle(luaPath, reportPath, repeatCount);
            return 0;
        }
    }

    SteamErrMsg err;
    /* doubt this will have actual effect */
    constexpr int gamePort = 1826;
    constexpr int queryPort = 1425;
    /* if queryPort is not open, you are running duplicate instances. */
    if(SteamGameServer_InitEx(
            INADDR_ANY, gamePort, queryPort,
            eServerModeAuthenticationAndSecure, "0.60.0", &err)
        != k_ESteamAPIInitResult_OK) {
        qCritical() << err;
        qFatal() <<
            "Fatal Error - "
            "SteamGameServer_Init() failed.";
        return STEAM_ERROR;
    }

    QT_USE_NAMESPACE
#if defined (Q_OS_WIN)
    KP::winConsoleCheck();
#endif

    Server server(argc, argv);

    server.setApplicationName("FleetMemories Server");
    server.setApplicationVersion("0.60.1"); // temp
    server.setOrganizationName("Harusame Software");
    server.setOrganizationDomain("fleetmemories.moe"); // temp
    settings = std::make_unique<QSettings>(new QSettings);

#if defined(Q_OS_UNIX)
    //setlocale(LC_NUMERIC, "C");
#endif

    QTranslator translator;
#ifdef QT_NO_DEBUG
    /*
    if(!settings->contains("server/language")) {
        QString steamLanguage = SteamGameServerUtils()->GetSteamUILanguage();
        QMap<QString, QString> LanguageView;
        LanguageView["english"] = QStringLiteral("en_US");
        LanguageView["schinese"] = QStringLiteral("zh_CN");
        LanguageView["japanese"] = QStringLiteral("ja_JP");
        if(LanguageView.contains(steamLanguage)) {
            settings->setValue("server/language", LanguageView[steamLanguage]);
        }
        else {
            qWarning() << "Language not natively supported";
        }
    }*/
#else
    settings->setValue("server/language", "en_US");
#endif

    QStringList uiLanguages = QLocale::system().uiLanguages();
    if(settings->contains("server/language")) {
        uiLanguages.prepend(settings->value("server/language").toString());
    }
    for (const QString &locale : uiLanguages) {
        const QString baseName = "FleetMemories_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            server.installTranslator(&translator);
            break;
        }
    }
    KP::initLog(true);

    QConsoleListener console(true);
    bool success = QObject::connect(&console, &QConsoleListener::newLine,
                                    &server, &Server::parse);
    if(!success) {
        throw std::runtime_error("Connection with input parser failed!");
    }
    qInstallMessageHandler(server.customMessageHandler);
    QTimer::singleShot(std::chrono::milliseconds(0), &server, &Server::openingwords);
    QTimer::singleShot(std::chrono::milliseconds(
                           settings->value(
                                       "server/displaypromptdelay", 100).toInt()),
                       &server, &Server::displayPrompt);

    // ↓ Start event loop
    int execvalue = server.exec();

    // ↓ Steam shutdown
    SteamGameServer_Shutdown();

    return execvalue;
}
