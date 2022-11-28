/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
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

#ifndef CLIENT_H
#define CLIENT_H

#include <QtNetwork>
#include "commandline.h"

class Client : public CommandLine {
    Q_OBJECT

public:
    explicit Client(int, char **);
    ~Client() noexcept;

public slots:
    void catbomb();
    void displayPrompt() override;
    bool parseSpec(const QStringList &) override;
    void serverResponse(const QString &, const QByteArray &);
    Q_DECL_DEPRECATED void update();

signals:
    void turnOffEchoing();
    void turnOnEchoing();

private slots:
    void encrypted();
    void errorOccurred(QAbstractSocket::SocketError);
    void handshakeInterrupted(const QSslError &);
    void pskRequired(QSslPreSharedKeyAuthenticator *);
    void readyRead();
    void shutdown();

private:
    void doDevelop(const QStringList &);
    void doFetch(const QStringList &);
    void doSwitch(const QStringList &);
    void exitGraceSpec() override;
    QString gameStateString() const;
    const QStringList getCommandsSpec() const override;
    const QStringList getValidCommands() const override;
    bool loggedIn() const;
    void parseConnectReq(const QStringList &);
    void parseDisconnectReq();
    bool parseGameCommands(const QString &, const QStringList &);
    void parsePassword(const QString &);
    void readWhenConnected(const QByteArray &);
    void readWhenUnConnected(const QByteArray &);
    void receivedAuth(const QJsonObject &);
    void receivedLogin(const QJsonObject &);
    void receivedLogout(const QJsonObject &);
    void receivedMsg(const QJsonObject &);
    void receivedReg(const QJsonObject &);

    QHostAddress address;
    quint16 port;

    QSslSocket socket;
    QSslConfiguration conf;

    unsigned int maxRetransmit;
    unsigned int retransmitTimes = 0;

    QString clientName;
    QString serverName;
    QByteArray password;

    bool attemptMode;
    bool registerMode;
    bool logoutPending;

    KP::GameState gameState;

    static const unsigned int defaultMaxRetransmit = 2;
    const QByteArray defaultSalt =
            QByteArrayLiteral("\xe8\xbf\x99\xe6\x98\xaf\xe4\xb8"
                              "\x80\xe6\x9d\xa1\xe5\x92\xb8\xe9"
                              "\xb1\xbc");
    Q_DISABLE_COPY(Client)
};

#endif // CLIENT_H
