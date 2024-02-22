#include "servermastersender.h"

ServerMasterSender::ServerMasterSender(QObject *parent)
    : QObject{parent}
{

}

void ServerMasterSender::addSender(QAbstractSocket *connection) {
    agents[connection] = new Sender(connection);
}

void ServerMasterSender::removeSender(QAbstractSocket *connection) {
    agents.remove(connection);
}

void ServerMasterSender::sendMessage(QAbstractSocket *connection,
                                     const QByteArray &contents) {
    if(agents.contains(connection))
        agents[connection]->enque(contents);
    else
        connection->write(contents);
}

int ServerMasterSender::numberofMembers() const {
    return agents.size();
}
