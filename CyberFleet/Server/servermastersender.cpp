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
                                     QByteArray contents) {
    if(agents.contains(connection))
        agents[connection]->enque(contents);
    else
        qWarning() << qtTrId("send-message-over-invalid-connection");
}

int ServerMasterSender::numberofMembers() {
    return agents.size();
}
