#include <QCoreApplication>
#include <QLocale>
#include <QTranslator>

#include <QTimer>

#include <iostream>

#include "console/runner.h"
#include "logic/strelations.h"



int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "test_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }


    STRelations::initRelations();

    Runner r;
    QObject::connect(&r, SIGNAL(finished()), &a, SLOT(quit()));
    QTimer::singleShot(0, &r, SLOT(run()));

    return a.exec();
}
