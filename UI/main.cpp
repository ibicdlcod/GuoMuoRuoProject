/* QtCreator generated */
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
#include <QTimer>

/* program header */
#include "serverrun.h"

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

    QCoreApplication a(argc, argv);

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "UI_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }
    try {
        if(argc <= 1 || std::strcmp(argv[1], "--client") == 0)
        {
            // client UI
        }
        else if(std::strcmp(argv[1], "--server") == 0)
        {
            // server UI
            ServerRun r;
            qInstallMessageHandler(ServerRun::customMessageHandler);
            QObject::connect(&r, &ServerRun::finished, &a, &QCoreApplication::quit);
            //QObject::connect(&r, &ServerRun::exit, &a, &QCoreApplication::exit, Qt::QueuedConnection);
            QTimer::singleShot(0, &r, &ServerRun::run);

            return a.exec();
        }
        else
        {
            throw std::invalid_argument("Either run without arguments or specify --client or --server!");
        }
    }  catch (std::invalid_argument &e) {
        qDebug() << e.what() << Qt::endl << "Press ENTER to exit.";
        return 1;
    }
    return -1;
}
