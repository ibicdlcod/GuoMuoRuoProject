#include <QCoreApplication>
#include <QLocale>
#include <QTranslator>
#include "qconsolelistener.h"

#include "client.h"
#include "kp.h"

QFile *logFile;
std::unique_ptr<QSettings> settings;

int main(int argc, char *argv[]) {
    QT_USE_NAMESPACE
#if defined (Q_OS_WIN)
    KP::winConsoleCheck();
#endif

    Client client(argc, argv);

#pragma message(NOT_M_CONST)
    client.setApplicationName("WrathofAdmiral2");
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

    QConsoleListener console(true);
    bool success = QObject::connect(&console, &QConsoleListener::newLine,
                                    &client, &Client::parse)
            && QObject::connect(&client, &Client::turnOffEchoing,
                                &console, &QConsoleListener::turnOffEchoing)
            && QObject::connect(&client, &Client::turnOnEchoing,
                                &console, &QConsoleListener::turnOnEchoing);
    if(!success) {
        throw std::runtime_error("Connection with input parser failed!");
    }
    qInstallMessageHandler(client.customMessageHandler);
    QTimer::singleShot(0, &client, &Client::openingwords);
    QTimer::singleShot(100, &client, &Client::displayPrompt);
    return client.exec();
}
