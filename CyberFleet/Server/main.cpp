#include <QCoreApplication>
#include <QLocale>
#include <QTranslator>
#include "qconsolelistener.h"
#include "../steam/steam_api.h"

#include "../Protocol/kp.h"
#include "server.h"

QFile *logFile;
std::unique_ptr<QSettings> settings;

static const int STEAM_ERROR = 1;

int main(int argc, char *argv[]) {

    if(SteamAPI_RestartAppIfNecessary(2632870)) { // keep steam_appid.txt
        return STEAM_ERROR;
    }
    if(!SteamAPI_Init()) {
        qFatal("Fatal Error - Steam must be running to play this game (SteamAPI_Init() failed).\n");
        return STEAM_ERROR;
    }

    QT_USE_NAMESPACE
#if defined (Q_OS_WIN)
    KP::winConsoleCheck();
#endif

    Server server(argc, argv);

#pragma message(NOT_M_CONST)
    server.setApplicationName("CyberFleet Server");
    server.setApplicationVersion("0.57.1"); // temp
    server.setOrganizationName("Harusame Software");
    server.setOrganizationDomain("hsny.xyz"); // temp
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
        settings->setValue("language", LanguageView[steamLanguage]);
    }
    else {
        qWarning() << "Language not natively supported";
    }
#else
    settings->setValue("language", "en_US");
#endif

    QStringList uiLanguages = QLocale::system().uiLanguages();
    if(settings->contains("language")) {
        uiLanguages.prepend(settings->value("language").toString());
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
    QTimer::singleShot(100, &server, &Server::displayPrompt);
    return server.exec();
}
