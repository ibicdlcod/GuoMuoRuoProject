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
