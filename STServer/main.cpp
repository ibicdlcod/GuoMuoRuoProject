#include <QCoreApplication>
#include <QLocale>
#include <QTranslator>

#include <QTimer>

#include "run.h"

#ifdef Q_OS_WIN
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#else
#include <locale.h>
#endif

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
        const QString baseName = "STServer_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }

    Run r;
    qInstallMessageHandler(Run::customMessageHandler);
    QObject::connect(&r, SIGNAL(finished()), &a, SLOT(quit()));
    QTimer::singleShot(0, &r, SLOT(run()));

    return a.exec();
}
