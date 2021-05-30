#ifndef RUN_H
#define RUN_H

#include <QObject>

#include "consoletextstream.h"
#include "dtlsserver.h"

class Run : public QObject
{
    Q_OBJECT

public:
    explicit Run(QObject *parent = nullptr);
    static void customMessageHandler(QtMsgType, const QMessageLogContext &, const QString &);

signals:
    void finished();

private slots:
    void run();
    void addErrorMessage(const QString &);
    void addWarningMessage(const QString &);
    void addInfoMessage(const QString &);
    void addClientMessage(const QString &peerInfo, const QByteArray &datagram,
                          const QByteArray &plainText);

private:
    DtlsServer server;
    ConsoleTextStream qout;
};

#endif // RUN_H
