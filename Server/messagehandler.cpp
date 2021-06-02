#include "messagehandler.h"

MessageHandler::MessageHandler(QObject *parent)
    : QObject(parent)
{

}

void MessageHandler::errorMessage(const QString &message)
{
    qCritical("[ServerError] %s", message.toUtf8().constData());
}

void MessageHandler::warningMessage(const QString &message)
{
    qWarning("[ServerWarning] %s", message.toUtf8().constData());
}

void MessageHandler::infoMessage(const QString &message)
{
    qInfo("[ServerInfo] %s", message.toUtf8().constData());
}

void MessageHandler::addClientMessage(const QString &peerInfo, const QByteArray &datagram,
                      const QByteArray &plainText)
{
    static const QString formatter = QStringLiteral("\n---------------"
                                                    "\nA message from %1"
                                                    "\nDTLS datagram:\n%2"
                                                    "\nAs plain text:\n%3\n");

    const QString html = formatter.arg(peerInfo, QString::fromUtf8(datagram.toHex(' ')),
                                       QString::fromUtf8(plainText));
    //QTextStream qout = QTextStream(stdout);
    //qout << html;
    qInfo("%s", html.toUtf8().constData());
}
