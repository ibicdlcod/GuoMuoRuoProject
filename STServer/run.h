#ifndef RUN_H
#define RUN_H

#include <QObject>

#include "consoletextstream.h"
#include "dtlsserver.h"
#include "wcwidth.h"
#include "qprint.h"

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
    void listAvailableAddresses();

private:
    template<class T>
    void qls(const QList<T>);
    static int callength(const QString &, bool naive = false);
    static int callength(const QHostAddress &, bool naive = false);
    static const QString & strfiy(const QString &);
    static QString strfiy(const QHostAddress &);

    DtlsServer server;
    ConsoleTextStream qout;
    QList<QHostAddress> availableAddresses;
};

#endif // RUN_H
