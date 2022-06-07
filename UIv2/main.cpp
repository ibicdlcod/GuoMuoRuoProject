#include <QCoreApplication>
#include <QLocale>
#include <QTranslator>

/* OS Specific */
#if defined (Q_OS_WIN)
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#elif defined (Q_OS_UNIX)
#include <locale.h>
#endif

/* Qt Libs */
#include <QDateTime>
#include <QSettings>

/* C++ Libs */
// unused at the moment

/* Third party */
#include "qconsolelistener.h"

/* program headers */
#include "magic.h"
#include "cliclient.h"
#include "cliserver.h"

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

    try {

        bool clientActive = (argc <= 1 || std::strcmp(argv[1], "--client") == 0);
        bool serverActive = (argc > 1 && std::strcmp(argv[1], "--server") == 0);

        if(!clientActive && !serverActive)
        {
            throw std::invalid_argument("Either run without arguments or specify --client or --server!");
        }
        CLI *a = nullptr;
        if(clientActive)
        {
            a = new CliClient(argc, argv);
        }
        /* I wish for a elif. This else can be deleted but clazy will warn you memory leak in case
         * clientActive and serverActive is both true, which is impossible */
        else
        {
            if(serverActive)
            {
                a = new CliServer(argc, argv);
            }
        }
        if(Q_UNLIKELY(!a))
        {
            throw std::invalid_argument("Either run without arguments or specify --client or --server!");
        }
#if defined(Q_OS_UNIX)
        setlocale(LC_NUMERIC, "C");
#endif
        a->setApplicationName("SpearofTanaka");
        a->setApplicationVersion("0.0.0"); // temp
        a->setOrganizationName("Kantai Self-Governing Patriotic Committee");
        a->setOrganizationDomain("xxx.xyz"); // temp
        settings = new QSettings();

        QString logFileName;
        if(clientActive)
        {
            logFileName = settings->value("Client Log File", "ClientLog.log").toString();
        }
        else
        {
            if(serverActive)
            {
                logFileName = settings->value("Server Log File", "ServerLog.log").toString();
            }
        }
        logFile = new QFile(logFileName);
        if(Q_UNLIKELY(!logFile) || !logFile->open(QIODevice::WriteOnly | QIODevice::Append))
        {
            qFatal("Log file cannot be opened");
        }

        QTranslator translator;
        const QStringList uiLanguages = settings->value("Languages", QLocale::system().uiLanguages()).toStringList();

        for (const QString &locale : uiLanguages) {
            const QString baseName = "UIv2_" + QLocale(locale).name();
            if (translator.load(":/i18n/" + baseName)) {
                a->installTranslator(&translator);
                break;
            }
        }
        QConsoleListener console(true);
        bool success = QObject::connect(&console, &QConsoleListener::newLine, a, &CLI::parse);
        if(!success)
        {
            throw std::runtime_error("Connection with input parser failed!");
        }
        qInstallMessageHandler(a->customMessageHandler);
        QTimer::singleShot(0, a, &CLI::openingwords);
        QTimer::singleShot(100, a, &CLI::displayPrompt);

        int result = a->exec();
        delete a;
        delete settings;
        return result;
    }  catch (std::invalid_argument &e) {
        qDebug() << "[Invalid Arguments] " << e.what() << Qt::endl << "Press ENTER to exit.";
        return 1;
    }  catch (std::runtime_error &e) {
        qDebug() << "[Runtime Error] " << e.what() << Qt::endl << "Press ENTER to exit.";
        return 2;
    }  catch (std::exception &e) {
        qDebug() << e.what() << Qt::endl << "Press ENTER to exit.";
        return -1;
    }
}
