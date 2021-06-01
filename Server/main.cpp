#include <QCoreApplication>
#include <QLocale>
#include <QTranslator>

#include <QTextStream>

#include "dtlsserver.h"
#include "messagehandler.h"
#include "run.h"

int main(int argc, char *argv[])
{
    QT_USE_NAMESPACE

    QCoreApplication a(argc, argv);

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "Server_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }

    QStringList argv_l = QStringList();
    for(int i = 0; i < argc; ++i)
    {
        argv_l.append(argv[i]);
    }

    Run r(nullptr, argc, argv_l);
    QObject::connect(&r, &Run::finished, &a, &QCoreApplication::quit);
    QObject::connect(&r, &Run::exit, &a, &QCoreApplication::exit, Qt::QueuedConnection);
    QTimer::singleShot(0, &r, &Run::run);

    return a.exec();
}
