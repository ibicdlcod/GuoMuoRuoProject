#ifndef RUNNER_H
#define RUNNER_H

#include <QObject>

#include "logic/strelations.h"

class Runner : public QObject {
    Q_OBJECT

public slots:
    void run();
    void invalidCommand();
    void showAllCommands();
    void showCommands(const QList<STCType>);
    void showHelp(QStringList);

signals:
    void finished();

private:
    void qls(const QStringList);
    static bool lengthcmp(const QString &, const QString &);
};

#endif // RUNNER_H
