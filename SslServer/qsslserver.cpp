#include "qsslserver.h"
#include <QSslSocket>
#include <QSslCertificate>
#include <QSslKey>

QSslServer::QSslServer(QObject *parent)
    : QTcpServer{parent} {
    const auto certs =
            QSslCertificate::fromPath("default_ca.der", QSsl::Der,
                                      QSslCertificate::
                                      PatternSyntax::FixedString);
    if(certs.length() > 0) {
        const auto cert = certs.at(0);
        qDebug() << cert.issuerInfo(QSslCertificate::Organization);
    }
    else
        qFatal("NO CERT");
}

void QSslServer::incomingConnection(qintptr socketDescriptor) {
    QSslSocket *serverSocket = new QSslSocket;
    serverSocket->setLocalCertificateChain(certs);
    serverSocket->setPrivateKey("Zephyr");
    if (serverSocket->setSocketDescriptor(socketDescriptor)) {
        addPendingConnection(serverSocket);
        connect(serverSocket, &QSslSocket::encrypted, this, &QSslServer::ready);
        serverSocket->startServerEncryption();
        qDebug() << serverSocket->privateKey().toDer();
    } else {
        delete serverSocket;
    }
}

void QSslServer::ready() {
    qDebug() << "Ready\n";
}
