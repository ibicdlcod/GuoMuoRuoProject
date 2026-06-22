/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "ui/mainwindow.h"

#include <QApplication>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDebug>
#include <QFile>
#include <QLockFile>
#include <QMessageBox>
#include <QSettings>
#include <QStandardPaths>
#include <QStyleFactory>
#include <QSurfaceFormat>
#include <QTimer>
#include <QTranslator>
#include <chrono>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <optional>

#include "../steam/steam_api.h"

#include "../Protocol/kp.h"
#include "clientv2.h"
#include "ui/boxcenterfusionstyle.h"

QFile *logFile;
std::unique_ptr<QSettings> settings;

namespace {
const int INSTANCE_ERROR = 1;
const int STEAM_ERROR = 2;
constexpr quint64 aiIdPrefix = 0x4149000000000000ULL;
constexpr quint64 aiIdMask = 0x0000FFFFFFFFFFFFULL;

quint64 aiUserIdFromName(const QString &name) {
    QByteArray utf8 = name.toUtf8();
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(utf8);
    QByteArray result = hash.result();
    quint64 h = 0;
    for (int i = 0; i < static_cast<int>(sizeof(quint64)) && i < result.size();
         ++i) {
        h = (h << CHAR_BIT) | static_cast<quint8>(result[i]);
    }
    /* aiIdPrefix uses 'AI' prefix, keeps IDs visually distinct
     * from Steam IDs. */
    return aiIdPrefix | (h & aiIdMask);
}
}

int main(int argc, char *argv[]) {
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    QSurfaceFormat::setDefaultFormat(format);

    bool aiMode = false;
    QString aiName;
    QString aiServerIp = QStringLiteral("127.0.0.1");
    quint16 aiServerPort = KP::aiDefaultServerPort;
    for (int i = 1; i < argc; ++i) {
        QString arg = QString::fromLocal8Bit(argv[i]);
        if(arg == QStringLiteral("--ai") && i + 1 < argc) {
#if PRODUCTION_ENV
            //% "AI mode is disabled in production builds."
            qWarning() << qtTrId("ai-mode-disabled-production");
            ++i;
#else
            aiMode = true;
            aiName = QString::fromLocal8Bit(argv[++i]);
#endif
        }
        else if(arg == QStringLiteral("--server-ip") && i + 1 < argc) {
            aiServerIp = QString::fromLocal8Bit(argv[++i]);
        }
        else if(arg == QStringLiteral("--server-port") && i + 1 < argc) {
            aiServerPort = QString::fromLocal8Bit(argv[++i]).toUShort();
        }
    }

    /* Steam restart check - must be very early */
    if(!aiMode && SteamAPI_RestartAppIfNecessary(KP::steamAppId)) {
        return STEAM_ERROR;
    }

#if defined(Q_OS_LINUX)
    // Auto-detect input method module based on XMODIFIERS
    if (qEnvironmentVariableIsEmpty("QT_IM_MODULE")) {
        const char* xmod = std::getenv("XMODIFIERS");
        if (xmod && std::strstr(xmod, "fcitx")) {
            qputenv("QT_IM_MODULE", "fcitx");
            qputenv("QT_IM_MODULES", "wayland;fcitx");
        } else if (xmod && std::strstr(xmod, "ibus")) {
            qputenv("QT_IM_MODULE", "ibus");
        }
        // Otherwise let Qt use default platform input context
    }
    qDebug() << "Using input method module:" << qgetenv("QT_IM_MODULE");
#endif

    std::unique_ptr<QCoreApplication> coreApp;
    std::unique_ptr<QApplication> guiApp;
    QCoreApplication *app = nullptr;
    if(aiMode) {
        coreApp = std::make_unique<QCoreApplication>(argc, argv);
        app = coreApp.get();
    }
    else {
        guiApp = std::make_unique<QApplication>(argc, argv);
        app = guiApp.get();
    }
    QCoreApplication &client = *app;
    std::unique_ptr<QTranslator> translator;
    std::optional<QLockFile> lockFile;
    if(!aiMode) {
        guiApp->setWindowIcon(QIcon(":/resources/icon.ico"));
    }
    /* Metadata */
    client.setApplicationName("FleetMemories");
    client.setApplicationVersion("0.60.1"); // temp
    client.setOrganizationName("Harusame Software");
    client.setOrganizationDomain("fleetmemories.moe"); // temp
    /* End Metadata */

    /* Steam initialization */
    if(!aiMode && !SteamAPI_Init()) {
        qFatal() <<
            "Fatal Error - Steam must be running to play this game "
            "(SteamAPI_Init() failed).\n";
        return STEAM_ERROR;
    }
    /* End Steam initialization */

    settings = std::make_unique<QSettings>();

    /* Display style */
    if(!aiMode) {
        BoxCenterFusionStyle *style = new BoxCenterFusionStyle();
        style->setBaseStyle(QStyleFactory::create("Fusion"));
        QApplication::setStyle(style);
    }

    /* Multilingual Support */
    if(!aiMode) {
#if defined(Q_OS_UNIX)
//    setlocale(LC_NUMERIC, "C");
#endif

        translator = std::make_unique<QTranslator>();
        if(!(settings->contains("client/language"))) {
            QString steamLanguage = SteamUtils()->GetSteamUILanguage();
            QMap<QString, QString> LanguageView;
            LanguageView["english"] = QStringLiteral("en_US");
            LanguageView["schinese"] = QStringLiteral("zh_CN");
            LanguageView["japanese"] = QStringLiteral("ja_JP");
            if(LanguageView.contains(steamLanguage)) {
                settings->setValue("client/language",
                                   LanguageView[steamLanguage]);
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
            if (translator->load(":/i18n/" + baseName)) {
                client.installTranslator(translator.get());
                break;
            }
        }
    }
    else {
        translator = std::make_unique<QTranslator>();
        if(translator->load(":/i18n/FleetMemories_en_US")) {
            client.installTranslator(translator.get());
        }
    }

    /* Single instance check */
    if(!aiMode) {
        QString lockPath =
            QStandardPaths::writableLocation(QStandardPaths::TempLocation)
            + "/" + client.applicationName() + ".lock";
        lockFile.emplace(lockPath);
        if (!lockFile->tryLock()) {
            //% "Another instance of FleetMemories is already running."
            QMessageBox::warning(nullptr, client.applicationName(),
                                 qtTrId("client-instance-already-running"));
            return INSTANCE_ERROR;
        }
    }

    KP::initLog(false);
    qInstallMessageHandler(customMessageHandler);
    /* End Multilingual Support */

    Client &clientInstance = Client::getInstance();
    if(aiMode) {
        clientInstance.aiMode = true;
        clientInstance.aiName = aiName;
        clientInstance.aiUserId = aiUserIdFromName(aiName);
        clientInstance.aiServerIp = aiServerIp;
        clientInstance.aiServerPort = aiServerPort;
    }

    /* GUI */
    std::unique_ptr<MainWindow> mainWindow;
    if(!aiMode) {
        mainWindow = std::make_unique<MainWindow>(nullptr, argc, argv);
#if defined(Q_OS_UNIX)
        //mainWindow->setWindowFlags(Qt::FramelessWindowHint);
#endif
        mainWindow->show();
    }
    /* End GUI */

    using namespace std::chrono_literals;
    if(aiMode) {
        QObject::connect(&clientInstance, &Client::qout,
                         [](const QString &msg, QColor, QColor) {
                             QTextStream out(stdout);
                             out << msg << Qt::endl;
                         });
        QObject::connect(&clientInstance, &Client::receivedResourceGainInfo,
                         [&clientInstance](const QJsonObject &info) {
                             //% "Resource gain cache refreshed, %1 entries."
                             emit clientInstance.qout(
                                 qtTrId("ai-resource-gain-refreshed")
                                     .arg(info.size()));
                         });
        QObject::connect(&clientInstance, &Client::mapSupremacyChanged,
                         [&clientInstance]() {
                             //% "Map supremacy cache refreshed, %1 entries."
                             emit clientInstance.qout(
                                 qtTrId("ai-map-supremacy-refreshed")
                                     .arg(clientInstance.mapSupremacies.size()));
                         });
        QTimer::singleShot(0ms, &clientInstance, &Client::aiAutoConnect);
    }

    // ↓ Start event loop
    int execvalue = client.exec();

    // ↓ Steam shutdown
    if(!aiMode) {
        SteamAPI_Shutdown();
    }

    return execvalue;
}
