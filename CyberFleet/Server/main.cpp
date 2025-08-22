#include <QCoreApplication>
#include <QLocale>
#include <QTranslator>
#include "../steam/steam_api.h"
#include "../steam/steam_gameserver.h"

#include "../Protocol/kp.h"
#include "qconsolelistener.h"
#include "server.h"

QFile *logFile;
std::unique_ptr<QSettings> settings;

namespace {
const int STEAM_ERROR = 1;
}

int main(int argc, char *argv[]) {

    if(SteamAPI_RestartAppIfNecessary(KP::steamAppId)) { // keep steam_appid.txt
        return STEAM_ERROR;
    }
    if(!SteamAPI_Init()) {
        /* ignore it, since servers need not to have steam client running */
    }

    QT_USE_NAMESPACE
#if defined (Q_OS_WIN)
    KP::winConsoleCheck();
#endif

    Server server(argc, argv);

#pragma message(NOT_M_CONST)
    server.setApplicationName("CyberFleet Server");
    server.setApplicationVersion("0.58.1"); // temp
    server.setOrganizationName("Harusame Software");
    server.setOrganizationDomain("harusoft.xyz"); // temp
    settings = std::make_unique<QSettings>(new QSettings);

#if defined(Q_OS_UNIX)
    //setlocale(LC_NUMERIC, "C");
#endif

    QTranslator translator;
#ifdef QT_NO_DEBUG
    QString steamLanguage = SteamUtils()->GetSteamUILanguage();
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
#else
    settings->setValue("server/language", "en_US");
#endif

    QStringList uiLanguages = QLocale::system().uiLanguages();
    if(settings->contains("server/language")) {
        uiLanguages.prepend(settings->value("server/language").toString());
    }
    for (const QString &locale : uiLanguages) {
        const QString baseName = "CyberFleet2_" + QLocale(locale).name();
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
    return server.exec();
}
