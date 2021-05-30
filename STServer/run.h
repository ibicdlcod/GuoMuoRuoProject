#ifndef RUN_H
#define RUN_H

#include <QObject>

#include "dtlsserver.h"

class Run : public QObject
{
    Q_OBJECT

public:
    explicit Run(QObject *parent = nullptr);

signals:
    void finished();

private slots:
    void run();
    void addErrorMessage(const QString &message);
    void addWarningMessage(const QString &message);
    void addInfoMessage(const QString &message);
    void addClientMessage(const QString &peerInfo, const QByteArray &datagram,
                          const QByteArray &plainText);

private:
    DtlsServer server;
};

#endif // RUN_H
