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
#include "server.h"

QFile *logFile;
QSettings *settings;

int main(int argc, char *argv[])
{
    QT_USE_NAMESPACE
#if defined (Q_OS_WIN)
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE)
    {
        throw GetLastError();
    }

    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode))
    {
        throw GetLastError();
    }

    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (!SetConsoleMode(hOut, dwMode))
    {
        throw GetLastError();
    }
#endif

    Server server(argc, argv);

#pragma message(NOT_M_CONST)
    server.setApplicationName("WrathofAdmiral2 Server");
    server.setApplicationVersion("0.54.1"); // temp
    server.setOrganizationName("Harusame Software");
    server.setOrganizationDomain("hsny.xyz"); // temp
    settings = new QSettings();

    QTranslator translator;
    settings->setValue("languages", QStringList("zh_CN")); // this is for testing
    const QStringList uiLanguages = QLocale::system().uiLanguages()
            + settings->value("languages", QStringList()).toStringList();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "WA2_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            server.installTranslator(&translator);
            break;
        }
    }

    QStringList argv_l = QStringList();
    for(int i = 0; i < argc; ++i)
    {
        argv_l.append(argv[i]);
    }
#if defined(Q_OS_UNIX)
    setlocale(LC_NUMERIC, "C");
#endif

    QString logFileName = settings->value("server/logfile", "ServerLog.log").toString();

    logFile = new QFile(logFileName);
    if(Q_UNLIKELY(!logFile) || !logFile->open(QIODevice::WriteOnly | QIODevice::Append))
    {
        qFatal("Log file cannot be opened");
    }

    QConsoleListener console(true);
    bool success = QObject::connect(&console, &QConsoleListener::newLine, &server, &Server::parse);
    if(!success)
    {
        throw std::runtime_error("Connection with input parser failed!");
    }
    qInstallMessageHandler(server.customMessageHandler);
    QTimer::singleShot(0, &server, &Server::openingwords);
    QTimer::singleShot(100, &server, &Server::displayPrompt);

    return server.exec();
}
