/**
 * Qt-SslServer, a Tcp Server class with SSL support using QTcpServer and QSslSocket.
 * Copyright (C) 2014  TRUCHOT Guillaume
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "sslserver.h"

#include <QFile>
#include <QSslSocket>

SslServer::SslServer(QObject *parent) : QSslServer{parent} {
    serverConfiguration = QSslConfiguration::defaultConfiguration();
    serverConfiguration.setPeerVerifyMode(QSslSocket::VerifyNone);
}

QByteArray SslServer::preSharedKeyIdentityHint() {
    return serverConfiguration.preSharedKeyIdentityHint();
}

void SslServer::incomingConnection(qintptr socketDesc) {
    // Create socket
    QSslSocket *sslSocket = new QSslSocket(this);
    sslSocket->setSocketDescriptor(socketDesc);
    sslSocket->setSslConfiguration(serverConfiguration);
    sslSocket->startServerEncryption();

    this->addPendingConnection(sslSocket);

    connect(sslSocket, &QSslSocket::readyRead,
                     this, &SslServer::readyRead);
}

void SslServer::setSslConfiguration(const QSslConfiguration &conf) {
    serverConfiguration = conf;
}

void SslServer::readyRead(){
    emit connectionReadyread(reinterpret_cast<QSslSocket *>
                             (QObject::sender()));
}
