/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "dtlsclient.h"

#include <QSettings>

#include "magic.h"

QT_BEGIN_NAMESPACE

extern QSettings *settings;

DtlsClient::DtlsClient(const QHostAddress &address, quint16 port,
                       const QString &connectionName)
    : name(connectionName),
      crypto(QSslSocket::SslClientMode),
      maxretransmit(settings->value("Client maximum restransmit", 5).toInt()),
      login_success(false)
{
    //! [1]
    auto configuration = QSslConfiguration::defaultDtlsConfiguration();
    configuration.setPeerVerifyMode(QSslSocket::VerifyNone);
    crypto.setPeer(address, port);
    crypto.setDtlsConfiguration(configuration);
    //! [1]

    //! [2]
    connect(&crypto, &QDtls::handshakeTimeout, this, &DtlsClient::handshakeTimeout);
    //! [2]
    connect(&crypto, &QDtls::pskRequired, this, &DtlsClient::pskRequired);
    //! [3]
    socket.connectToHost(address.toString(), port);
    //! [3]
    //! [13]
    connect(&socket, &QUdpSocket::readyRead, this, &DtlsClient::readyRead);
    //! [13]
    //! [4]
    pingTimer.setInterval(5000);
    connect(&pingTimer, &QTimer::timeout, this, &DtlsClient::pingTimeout);
    //! [4]
}

//! [12]
DtlsClient::~DtlsClient()
{
    if (crypto.isConnectionEncrypted())
        crypto.shutdown(&socket);
}
//! [12]

//! [5]
void DtlsClient::startHandshake()
{
    if (socket.state() != QAbstractSocket::ConnectedState) {
        infoMessage(tr("%1: connecting UDP socket first ...").arg(name));
        connect(&socket, &QAbstractSocket::connected, this, &DtlsClient::udpSocketConnected);
        return;
    }

    if (!crypto.doHandshake(&socket))
        errorMessage(tr("%1: failed to start a handshake - %2").arg(name, crypto.dtlsErrorString()));
    else
        infoMessage(tr("%1: starting a handshake").arg(name));
}
//! [5]

void DtlsClient::udpSocketConnected()
{
    infoMessage(tr("%1: UDP socket is now in ConnectedState, continue with handshake ...").arg(name));
    startHandshake();

    retransmit_times = 0;
}

void DtlsClient::readyRead()
{
    if (socket.pendingDatagramSize() <= 0) {
        warningMessage(tr("%1: spurious read notification?").arg(name));
        return;
    }

    //! [6]
    QByteArray dgram(socket.pendingDatagramSize(), Qt::Uninitialized);
    const qint64 bytesRead = socket.readDatagram(dgram.data(), dgram.size());
    if (bytesRead <= 0) {
        warningMessage(tr("%1: spurious read notification?").arg(name));
        return;
    }

    dgram.resize(bytesRead);
    //! [6]
    //! [7]
    if (crypto.isConnectionEncrypted()) {
        const QByteArray plainText = crypto.decryptDatagram(&socket, dgram);
        if (plainText.size()) {
            serverResponse(name, dgram, plainText);
            return;
        }

        if (crypto.dtlsError() == QDtlsError::RemoteClosedConnectionError) {
            errorMessage(tr("%1: shutdown alert received").arg(name));
            socket.close();
            pingTimer.stop();
            catbomb();
            return;
        }

        warningMessage(tr("%1: zero-length datagram received?").arg(name));
    } else {
        //! [7]
        //! [8]
        if (!crypto.doHandshake(&socket, dgram)) {
            errorMessage(tr("%1: handshake error - %2").arg(name, crypto.dtlsErrorString()));
            return;
        }
        //! [8]

        //! [9]
        if (crypto.isConnectionEncrypted()) {
            infoMessage(tr("%1: encrypted connection established!").arg(name));
            login_success = true;
            pingTimer.start();
            pingTimeout();
        } else {
            //! [9]
            infoMessage(tr("%1: continuing with handshake ...").arg(name));
        }
    }
}

//! [11]
void DtlsClient::handshakeTimeout()
{
    warningMessage(tr("%1: handshake timeout, trying to re-transmit").arg(name));
    retransmit_times++;
    if (!crypto.handleTimeout(&socket))
        errorMessage(tr("%1: failed to re-transmit - %2").arg(name, crypto.dtlsErrorString()));
    if(retransmit_times > maxretransmit)
    {
        errorMessage(tr("%1: max restransmit time exceeded!").arg(name));
        catbomb();
    }
}
//! [11]

//! [14]
void DtlsClient::pskRequired(QSslPreSharedKeyAuthenticator *auth)
{
    Q_ASSERT(auth);

    infoMessage(tr("%1: providing pre-shared key ...").arg(name));
    auth->setIdentity(name.toLatin1());
#pragma message(M_CONST)
    auth->setPreSharedKey(QByteArrayLiteral("register"));

}
//! [14]

//! [10]
void DtlsClient::pingTimeout()
{/*
    static const QString message = QStringLiteral("PING %1 ATTEMPT %2");
    const qint64 written = crypto.writeDatagramEncrypted(&socket, message.arg(name).arg(ping).toLatin1());
    if (written <= 0) {
        errorMessage(tr("%1: failed to send a ping - %2").arg(name, crypto.dtlsErrorString()));
        pingTimer.stop();
        return;
    }

    ++ping;
    */
}
//! [10]

void DtlsClient::catbomb()
{
    /* no tr() should be used here */
    if(login_success)
    {
        errorMessage("[CATBOMB]");
    }
    else
    {
        errorMessage(tr("Failed to establish connection, check your username, password and server status."));
    }
    QTimer::singleShot(500, this, &DtlsClient::finished);
}

void DtlsClient::errorMessage(const QString &message)
{
    qCritical("[ClientError] %s", message.toUtf8().constData());
}

void DtlsClient::warningMessage(const QString &message)
{
    qWarning("[ClientWarning] %s", message.toUtf8().constData());
}

void DtlsClient::infoMessage(const QString &message)
{
    qInfo("[ClientInfo] %s", message.toUtf8().constData());
}

void DtlsClient::serverResponse(const QString &clientInfo, const QByteArray &datagram,
                                const QByteArray &plainText)
{
    static const QString formatter = QStringLiteral("%1 received text: %2");

    const QString html = formatter.arg(clientInfo, QString::fromUtf8(plainText));
    qInfo("%s", html.toUtf8().constData());
}

void DtlsClient::parse(const QString &cmdline)
{
    if(cmdline.startsWith("SIGTERM"))
    {
        infoMessage(tr("received SIGTERM"));
        if (crypto.isConnectionEncrypted())
            crypto.shutdown(&socket);
        infoMessage(tr("Client is shutting down"));
        QTimer::singleShot(2000, this, &DtlsClient::finished);
    }
    if(cmdline.startsWith("REG"))
    {
        QStringList cmdParts = cmdline.split(" ");
        if(cmdParts.size() < 3)
        {
            warningMessage("You must specify your username and password");
            QCoreApplication::processEvents();
            return;
        }
        settings->setValue("Username", cmdParts[1]);
        static const QString message = QStringLiteral("REG %1 SHADOW %2");
        const qint64 written = crypto.writeDatagramEncrypted(&socket, message.arg(cmdParts[1]).arg(cmdParts[2]).toLatin1());

        if (written <= 0) {
            errorMessage(tr("%1: failed to send register attmpt - %2").arg(cmdParts[1], crypto.dtlsErrorString()));
            return;
        }
    }
}
QT_END_NAMESPACE
