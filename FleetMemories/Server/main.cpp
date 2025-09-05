#include <QCoreApplication>
#include <QLocale>
#include <QTranslator>
#include "../steam/steam_gameserver.h"

#include "../Protocol/kp.h"
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

#pragma message(NOT_M_CONST)
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
    }
#else
    settings->setValue("server/language", "zh_CN");
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
    QTimer::singleShot(0, &server, &Server::openingwords);
    QTimer::singleShot(settings->value("server/displaypromptdelay", 100).toInt(),
                       &server, &Server::displayPrompt);

    // ↓ Start event loop
    int execvalue = server.exec();

    // ↓ Steam shutdown
    SteamGameServer_Shutdown();

    return execvalue;
}
