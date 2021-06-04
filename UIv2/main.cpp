#include <QCoreApplication>
#include <QLocale>
#include <QTranslator>

/* OS Specific */
#ifdef Q_OS_WIN
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#else
#include <locale.h>
#endif

/* Qt Libs */
#include <QDateTime>

/* Third party */
#include "qconsolelistener.h"

/* program headers */
#include "cliserver.h"

QFile *logFile;

int main(int argc, char *argv[])
{
    QT_USE_NAMESPACE

#ifdef Q_OS_WIN
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
#else
    setlocale(LC_ALL, "");
#endif

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();

    try {
        if(argc <= 1 || std::strcmp(argv[1], "--client") == 0)
        {
            // client UI
            qDebug("Client has yet to be implimented.");
            return 0;
        }
        else if(std::strcmp(argv[1], "--server") == 0)
        {
            // server UI
            logFile = new QFile("LogFile.log");
            if(!logFile->open(QIODevice::WriteOnly | QIODevice::Append))
            {
                qFatal("Log file cannot be opened");
            }
            CliServer a(argc, argv);
            for (const QString &locale : uiLanguages) {
                const QString baseName = "UIv2_" + QLocale(locale).name();
                if (translator.load(":/i18n/" + baseName)) {
                    a.installTranslator(&translator);
                    break;
                }
            }
            QConsoleListener console(true);
            bool success = QObject::connect(&console, &QConsoleListener::newLine, &a, &CLI::parse);
            if(!success)
            {
                throw std::runtime_error("Connection with input parser failed!");
            }
            qInstallMessageHandler(a.customMessageHandler);
            QTimer::singleShot(0, &a, &CLI::openingwords);
            QTimer::singleShot(0, &a, &CLI::displayPrompt);

            return a.exec();
        }
        else
        {
            throw std::invalid_argument("Either run without arguments or specify --client or --server!");
        }
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
