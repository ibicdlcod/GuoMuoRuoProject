#ifndef RECIVER_H
#define RECIVER_H

#include <QAbstractSocket>
#include <QUuid>

class Receiver : public QObject
{
    Q_OBJECT
public:
    explicit Receiver(QObject *parent = nullptr);

signals:
    void jsonReceived(const QJsonObject &djson);
    void nonJsonRecived(const QByteArray &dgram);
    void timeOut(QUuid);

public slots:
    void processDgram(const QByteArray &);

private slots:
    void processGoodMsg(qint64, qint64, QUuid, const QString &);

private:
    QMap<QUuid, QList<QString>> receivedStatus;
    QMap<QUuid, qint64> totalPartsMap;
    QMap<QUuid, qint64> receivedPartsMap;
};

#endif // SENDER_H
