#ifndef RECIVER_H
#define RECIVER_H

#include <QAbstractSocket>
#include <QUuid>
#include <QTimer>
#include <bitset>

// no more than 200 parts will be used for 537 equip defs
static const int maxNumberOfParts = 1024;

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
    QMap<QUuid, std::bitset<maxNumberOfParts>> receivedPartsMap;
    QMap<QUuid, QTimer *> timers;
};

#endif // SENDER_H
