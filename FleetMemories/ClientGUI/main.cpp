/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "ui/mainwindow.h"

#include <QApplication>
#include <QFile>
#include <QSettings>
#include <QStyleFactory>
#include <QSurfaceFormat>
#include <QTranslator>
#include <QDebug>
#include <cstdlib>
#include <cstring>

#include "../steam/steam_api.h"

#include "../Protocol/kp.h"
#include "clientv2.h"
#include "ui/boxcenterfusionstyle.h"

QFile *logFile;
std::unique_ptr<QSettings> settings;

namespace {
const int STEAM_ERROR = 1;
}

int main(int argc, char *argv[]) {
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    QSurfaceFormat::setDefaultFormat(format);

    /* Steam initialization */
    if(SteamAPI_RestartAppIfNecessary(KP::steamAppId)) {
        return STEAM_ERROR;
    }
    if(!SteamAPI_Init()) {
        qFatal() <<
            "Fatal Error - Steam must be running to play this game "
            "(SteamAPI_Init() failed).\n";
        return STEAM_ERROR;
    }
    /* End Steam initialization */

#if defined(Q_OS_LINUX)
    // Auto-detect input method module based on XMODIFIERS
    if (qEnvironmentVariableIsEmpty("QT_IM_MODULE")) {
        const char* xmod = std::getenv("XMODIFIERS");
        if (xmod && std::strstr(xmod, "fcitx")) {
            qputenv("QT_IM_MODULE", "fcitx");
        } else if (xmod && std::strstr(xmod, "ibus")) {
            qputenv("QT_IM_MODULE", "ibus");
        }
        // Otherwise let Qt use default platform input context
    }
    qDebug() << "Using input method module:" << qgetenv("QT_IM_MODULE");
#endif

    QApplication client(argc, argv);
    client.setWindowIcon(QIcon(":/resources/icon.ico"));
#pragma message(NOT_M_CONST)
    /* Metadata */
    client.setApplicationName("FleetMemories");
    client.setApplicationVersion("0.60.1"); // temp
    client.setOrganizationName("Harusame Software");
    client.setOrganizationDomain("fleetmemories.moe"); // temp
    /* End Metadata */

    settings = std::make_unique<QSettings>();

    /* Display style */
    BoxCenterFusionStyle *style = new BoxCenterFusionStyle();
    style->setBaseStyle(QStyleFactory::create("Fusion"));
    QApplication::setStyle(style);

    /* Multilingual Support */
#if defined(Q_OS_UNIX)
//    setlocale(LC_NUMERIC, "C");
#endif

    QTranslator translator;
    if(!(settings->contains("client/language"))) {
        QString steamLanguage = SteamUtils()->GetSteamUILanguage();
        QMap<QString, QString> LanguageView;
        LanguageView["english"] = QStringLiteral("en_US");
        LanguageView["schinese"] = QStringLiteral("zh_CN");
        LanguageView["japanese"] = QStringLiteral("ja_JP");
        if(LanguageView.contains(steamLanguage)) {
            settings->setValue("client/language", LanguageView[steamLanguage]);
        }
        else {
            qWarning() << "Steam language not natively supported";
        }
    }

    QStringList uiLanguages = QLocale::system().uiLanguages();
    if(settings->contains("client/language")) {
        uiLanguages.prepend(settings->value("client/language").toString());
    }
    for (const QString &locale : uiLanguages) {
        const QString baseName = "FleetMemories_" + QLocale(locale).name();
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
#if defined(Q_OS_UNIX)
    //w.setWindowFlags(Qt::FramelessWindowHint);
#endif
    w.show();
    /* End GUI */

    // ↓ Start event loop
    int execvalue = client.exec();

    // ↓ Steam shutdown
    SteamAPI_Shutdown();

    return execvalue;
}
