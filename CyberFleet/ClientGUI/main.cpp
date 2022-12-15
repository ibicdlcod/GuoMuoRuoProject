#include "mainwindow.h"

#include <QApplication>
#include <QFile>
#include <QSettings>
#include <QTranslator>
#include "clientv2.h"
#include "kp.h"

QFile *logFile;
std::unique_ptr<QSettings> settings;

int main(int argc, char *argv[])
{
    QApplication client(argc, argv);

#pragma message(NOT_M_CONST)
    client.setApplicationName("CyberFleet");
    client.setApplicationVersion("0.55.1"); // temp
    client.setOrganizationName("Harusame Software");
    client.setOrganizationDomain("hsny.xyz"); // temp
    settings = std::make_unique<QSettings>(new QSettings);

#if defined(Q_OS_UNIX)
    setlocale(LC_NUMERIC, "C");
#endif

    QTranslator translator;
    /* For testing purposes */
    settings->setValue("language", QStringLiteral("zh_CN"));

    QStringList uiLanguages = QLocale::system().uiLanguages();
    if(settings->contains("language")) {
        uiLanguages.prepend(settings->value("language").toString());
    }
    for (const QString &locale : uiLanguages) {
        const QString baseName = "WA2_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            client.installTranslator(&translator);
            break;
        }
    }
    KP::initLog(false);
    qInstallMessageHandler(customMessageHandler);

    MainWindow w(nullptr, argc, argv);
    w.show();
    return client.exec();
}
