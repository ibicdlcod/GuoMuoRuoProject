#include <QCoreApplication>
#include <QLocale>
#include <QTranslator>
#include <QSettings>
#include <QFile>
#include "qconsolelistener.h"

/* OS Specific */
#if defined (Q_OS_WIN)
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#elif defined (Q_OS_UNIX)
#include <locale.h>
#endif

#include "kp.h"
#include "client.h"

QFile *logFile;
QSettings *settings;

void initLog() {
    QString logFileName= settings->value("client/logfile",
                                         "ClientLog.log").toString();
    logFile = new QFile(logFileName);
    if(Q_UNLIKELY(!logFile)
            || !logFile->open(QIODevice::WriteOnly | QIODevice::Append)) {
        qFatal("Log file cannot be opened");
    }

}

void winConsoleCheck() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) {
        throw GetLastError();
    }
    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) {
        throw GetLastError();
    }
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (!SetConsoleMode(hOut, dwMode)) {
        throw GetLastError();
    }
}

int main(int argc, char *argv[]) {
    QT_USE_NAMESPACE
#if defined (Q_OS_WIN)
    winConsoleCheck();
#endif

    Client client(argc, argv);

#pragma message(NOT_M_CONST)
    client.setApplicationName("WrathofAdmiral2");
    client.setApplicationVersion("0.55.1"); // temp
    client.setOrganizationName("Harusame Software");
    client.setOrganizationDomain("hsny.xyz"); // temp
    settings = new QSettings();

#if defined(Q_OS_UNIX)
    setlocale(LC_NUMERIC, "C");
#endif

    QTranslator translator;
    /* For testing purposes */
    settings->setValue("languages", QStringList("zh_CN"));

    QStringList uiLanguages = QLocale::system().uiLanguages();
    uiLanguages.append(settings->value("languages", QStringList())
                       .toStringList());
    for (const QString &locale : uiLanguages) {
        const QString baseName = "WA2_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            client.installTranslator(&translator);
            break;
        }
    }
    initLog();

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
