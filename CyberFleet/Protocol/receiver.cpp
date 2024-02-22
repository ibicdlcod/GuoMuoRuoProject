#include "receiver.h"
#include <QRegularExpression>
#include <QJsonObject>
#include <QCborMap>

static const int maxMsgDelayInMs = 1000;

Receiver::Receiver(QObject *parent)
    : QObject(parent) {

}

void Receiver::processDgram(const QByteArray &input) {
    /* communication between client and server using non-latin1
     * should be banned in this program */
    QString inputString = QString::fromLatin1(input);
    static QRegularExpression re("\\x01(\\d+)\\x09(\\d+)\\x09(.+?)\\x09(.+?)\\x7f",
                                 QRegularExpression::DotMatchesEverythingOption);

    QRegularExpressionMatchIterator i = re.globalMatch(inputString);
    while(i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        if (match.hasMatch()) {
            qint64 totalParts = match.captured(1).toInt();
            qint64 currentPart = match.captured(2).toInt();
            QUuid uuid = QUuid(match.captured(3));
            QString contents = match.captured(4);
            processGoodMsg(totalParts, currentPart, uuid, contents);
        }
    }
    inputString.replace(re, "");
    if(!inputString.isEmpty()) {
        emit nonStandardReceived(inputString.toLatin1());
    }
}

void Receiver::processGoodMsg(qint64 totalParts,
                              qint64 currentPart,
                              QUuid msgId,
                              const QString &contents) {
    if(!totalPartsMap.contains(msgId)) {
        totalPartsMap[msgId] = totalParts;
        receivedPartsMap[msgId] = 0;
        receivedStatus[msgId] = QList<QString>(totalParts);
        timers[msgId] = new QTimer(this);
        timers[msgId]->setSingleShot(true);
        connect(timers[msgId], &QTimer::timeout, this,
                [=]() {
                    // "Message %1 timeouted when receiving!"
                    qCritical() << qtTrId("receive-msg-timeout").arg(msgId.toString());
                    totalPartsMap.remove(msgId);
                    receivedPartsMap.remove(msgId);
                    receivedStatus.remove(msgId);
                    emit timeOut(msgId);
                }
                );
        timers[msgId]->start(maxMsgDelayInMs * totalParts);
    }
    else if(totalPartsMap[msgId] != totalParts)
        qCritical() << qtTrId("same-msg-uid-have-inconsistent-total-parts");

    Q_ASSERT(receivedPartsMap.contains(msgId));
    receivedPartsMap[msgId] += 1 << currentPart;
    Q_ASSERT(receivedStatus.contains(msgId));
    Q_ASSERT(receivedStatus[msgId].length() > currentPart);
    receivedStatus[msgId][currentPart] = contents;

    if((receivedPartsMap[msgId] + 1) == (1 << totalParts)) {
        QString wholeMessage = receivedStatus[msgId].join("");
        timers[msgId]->stop();

        QJsonObject djson =
            QCborValue::fromCbor(wholeMessage.toLatin1()).toMap().toJsonObject();
        if(!djson.isEmpty()) {
            emit jsonReceived(djson);
        }
        else
            qCritical() << qtTrId("msg-convert-to-json-failed");

        totalPartsMap.remove(msgId);
        receivedPartsMap.remove(msgId);
        receivedStatus.remove(msgId);
    }
}
