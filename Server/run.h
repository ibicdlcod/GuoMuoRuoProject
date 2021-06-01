#ifndef RUN_H
#define RUN_H

#include <QObject>

#include "dtlsserver.h"
#include "messagehandler.h"

class Run : public QObject
{
    Q_OBJECT
public:
    explicit Run(QObject *parent = nullptr, int argc = 0, QStringList argv = QStringList());

signals:
    void exit(int);
    void finished();

public slots:
    void run();

private:
    int argc;
    QStringList argv;
};

#endif // RUN_H
