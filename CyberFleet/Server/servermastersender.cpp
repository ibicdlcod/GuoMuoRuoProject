#include "servermastersender.h"

ServerMasterSender::ServerMasterSender(QObject *parent)
    : QObject{parent}
{

}

void ServerMasterSender::addSender(QAbstractSocket *connection) {
    agents[connection] = new Sender(connection);
    connect(agents[connection], &Sender::errorOccurred,
            this, &ServerMasterSender::errorHandle);
}

void ServerMasterSender::removeSender(QAbstractSocket *connection) {
    disconnect(agents[connection], &Sender::errorOccurred,
               this, &ServerMasterSender::errorHandle);
    agents.remove(connection);
}

void ServerMasterSender::sendMessage(QAbstractSocket *connection,
                                     const QByteArray &contents) {
    if(agents.contains(connection))
        agents[connection]->enque(contents);
    else
        connection->write(contents);
}

void ServerMasterSender::errorHandle(const QString &error) {
    Sender *individual = qobject_cast<Sender *>(QObject::sender());
    //% Address %1 Port %2 Errror: %3
    emit errorMessage(qtTrId("sender-error")
                           .arg(individual->peerAddress().toString())
                           .arg(individual->peerPort())
                           .arg(error));
}

int ServerMasterSender::numberofMembers() const {
    return agents.size();
}
