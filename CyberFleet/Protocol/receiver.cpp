#include "receiver.h"
#include <QRegularExpression>
#include <QJsonObject>
#include <QCborMap>

Receiver::Receiver(QObject *parent)
    : QObject(parent) {

}

void Receiver::processDgram(const QByteArray &input) {
    /* communication between client and server using non-latin1
     * should be banned in this program */
    QString inputString = QString::fromLatin1(input);
    static QRegularExpression re("\\x01(\\d+)\\x09(\\d+)\\x09(.+?)\\x09(.+?)\\x7f");
    qDebug() << re.isValid();

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
        emit nonJsonRecived(inputString.toLatin1());
        qDebug() << inputString;
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
        qInfo() << wholeMessage;

        QJsonObject djson =
            QCborValue::fromCbor(wholeMessage.toLatin1()).toMap().toJsonObject();
        if(!djson.isEmpty()) {
            emit jsonReceived(djson);
            qDebug() << djson;
        }
        else
            qCritical() << qtTrId("msg-convert-to-json-failed");

        totalPartsMap.remove(msgId);
        receivedPartsMap.remove(msgId);
        receivedStatus.remove(msgId);
    }
}
