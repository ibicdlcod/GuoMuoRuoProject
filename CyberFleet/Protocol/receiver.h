#ifndef RECIVER_H
#define RECIVER_H

#include <QAbstractSocket>
#include <QUuid>
#include <QTimer>
#include <QSslSocket>
#include <bitset>
#include "peerinfo.h"
#include "kp.h"

// Do not modify
#pragma message(NOT_M_CONST)
static const int maxNumberOfParts = 1024;

class Receiver : public QObject
{
    Q_OBJECT
public:
    explicit Receiver(QObject *parent = nullptr);

signals:
    void jsonReceived(const QJsonObject &);
    void jsonReceivedWithInfo(const QJsonObject &,
                              const PeerInfo &,
                              QSslSocket *);
    void nonStandardReceived(const QByteArray &);
    void nonStandardReceivedWithInfo(const QByteArray &,
                                     const PeerInfo &,
                                     QSslSocket *);
    /* should generate a catbomb, but currently don't */
    void timeOut(QUuid);

public slots:
    void processDgram(const QByteArray &);
    void processDgramWithInfo(const PeerInfo &,
                              const QByteArray &,
                              QSslSocket *);

private slots:
    void processGoodMsg(qint64, qint64, QUuid, const QString &);

private:
    QMap<QUuid, QList<QString>> receivedStatus;
    QMap<QUuid, qint64> totalPartsMap;
    QMap<QUuid, std::bitset<maxNumberOfParts>> receivedPartsMap;
    QMap<QUuid, QTimer *> timers;
    QMap<QUuid, PeerInfo> peerInfos;
    QMap<QUuid, QSslSocket *> sslsockets;
};

#endif // SENDER_H
