#ifndef RECIVER_H
#define RECIVER_H

#include <QAbstractSocket>
#include <QUuid>
#include <QTimer>

class Receiver : public QObject
{
    Q_OBJECT
public:
    explicit Receiver(QObject *parent = nullptr);

signals:
    void jsonReceived(const QJsonObject &djson);
    void nonStandardReceived(const QByteArray &dgram);
    /* should generate a catbomb */
    void timeOut(QUuid);

public slots:
    void processDgram(const QByteArray &);

private slots:
    void processGoodMsg(qint64, qint64, QUuid, const QString &);

private:
    QMap<QUuid, QList<QString>> receivedStatus;
    QMap<QUuid, qint64> totalPartsMap;
    QMap<QUuid, qint64> receivedPartsMap;
    QMap<QUuid, QTimer *> timers;
};

#endif // SENDER_H
