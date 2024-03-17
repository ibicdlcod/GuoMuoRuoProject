#include "ui/mainwindow.h"

#include <QApplication>
#include <QFile>
#include <QSettings>
#include <QTranslator>
#include <QStyleFactory>
#include "../steam/steam_api.h"

#include "clientv2.h"
#include "../Protocol/kp.h"
#include "ui/boxcenterfusionstyle.h"

QFile *logFile;
std::unique_ptr<QSettings> settings;

namespace {
const int STEAM_ERROR = 1;
}

int main(int argc, char *argv[]) {

    /* Steam initialization */
    if(SteamAPI_RestartAppIfNecessary(2632870)) {
        return STEAM_ERROR;
    }
    if(!SteamAPI_Init()) {
        qFatal() <<
            "Fatal Error - Steam must be running to play this game "
            "(SteamAPI_Init() failed).\n";
        return STEAM_ERROR;
    }
    /* End Steam initialization */
    QApplication client(argc, argv);
#if defined(Q_OS_UNIX)
    client.setWindowIcon(QIcon(":/resources/icon.ico"));
#endif
#pragma message(NOT_M_CONST)
    /* Metadata */
    client.setApplicationName("CyberFleet");
    client.setApplicationVersion("0.58.1"); // temp
    client.setOrganizationName("Harusame Software");
    client.setOrganizationDomain("harusoft.xyz"); // temp
    /* End Metadata */

    settings = std::make_unique<QSettings>(new QSettings);

    BoxCenterFusionStyle *style = new BoxCenterFusionStyle();
    style->setBaseStyle(QStyleFactory::create("Fusion"));
    QApplication::setStyle(style);

    /* Multilingual Support */
#if defined(Q_OS_UNIX)
//    setlocale(LC_NUMERIC, "C");
#endif

    QTranslator translator;
#ifdef QT_NO_DEBUG
    QString steamLanguage = SteamUtils()->GetSteamUILanguage();
    QMap<QString, QString> LanguageView;
    LanguageView["english"] = QStringLiteral("en_US");
    LanguageView["schinese"] = QStringLiteral("zh_CN");
    LanguageView["japanese"] = QStringLiteral("ja_JP");
    if(LanguageView.contains(steamLanguage)) {
        settings->setValue("client/language", LanguageView[steamLanguage]);
    }
    else {
        qWarning() << "Language not natively supported";
    }
#else
    settings->setValue("client/language", "ja_JP");
#endif

    QStringList uiLanguages = QLocale::system().uiLanguages();
    if(settings->contains("client/language")) {
        uiLanguages.prepend(settings->value("client/language").toString());
    }
    for (const QString &locale : uiLanguages) {
        const QString baseName = "CyberFleet2_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            client.installTranslator(&translator);
            break;
        }
    }
    KP::initLog(false);
    qInstallMessageHandler(customMessageHandler);
    /* End Multilingual Support */

    /* GUI */
    MainWindow w(nullptr, argc, argv);
    w.show();
    /* End GUI */

    // ↓ Start event loop
    int execvalue = client.exec();

    // ↓ Steam shutdown
    SteamAPI_Shutdown();

    return execvalue;
}
