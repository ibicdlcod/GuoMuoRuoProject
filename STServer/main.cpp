#include <QCoreApplication>
#include <QLocale>
#include <QTranslator>

#include <QTimer>

#include "run.h"

int main(int argc, char *argv[])
{
    QT_USE_NAMESPACE

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
    QObject::connect(&r, SIGNAL(finished()), &a, SLOT(quit()));
    QTimer::singleShot(0, &r, SLOT(run()));

    return a.exec();
}
