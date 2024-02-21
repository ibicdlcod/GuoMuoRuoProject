#include "servermastersender.h"

serverMasterSender::serverMasterSender(QObject *parent)
    : QObject{parent}
{

}

void serverMasterSender::addSender(QAbstractSocket *connection) {
    agents[connection] = new Sender(connection);
}

void serverMasterSender::removeSender(QAbstractSocket *connection) {
    agents.remove(connection);
}

void serverMasterSender::sendMessage(QAbstractSocket *connection,
                                     QByteArray contents) {
    agents[connection]->enque(contents);
}
