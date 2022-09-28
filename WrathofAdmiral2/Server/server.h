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

#ifndef SERVER_H
#define SERVER_H

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSslConfiguration>
#include <QtNetwork>
#include <memory>
#include <random>
#include <vector>

#include "commandline.h"
#include "equipment.h"
#include "peerinfo.h"
#include "user.h"

class Server : public CommandLine {
    Q_OBJECT

    typedef int Uid;

public:
    explicit Server(int, char **);
    ~Server() noexcept;

    void datagramReceived(const PeerInfo &, const QByteArray &, QDtls *);
    bool listen(const QHostAddress &, quint16);

public slots:
    void displayPrompt() override;
    bool parseSpec(const QStringList &) override;
    Q_DECL_DEPRECATED void update();

private slots:
    void pskRequired(QSslPreSharedKeyAuthenticator *);
    void readyRead();
    void shutdown();

private:
    void decryptDatagram(QDtls *, const QByteArray &);
    void doHandshake(QDtls *, const QByteArray &);
    bool equipmentRefresh();
    void exitGraceSpec() override;
    bool exportEquipToCSV() const;
    const QStringList getCommandsSpec() const override;
    const QStringList getValidCommands() const override;
    void handleNewConnection(const QHostAddress &, quint16,
                             const QByteArray &);
    bool importEquipFromCSV();
    void parseListen(const QStringList &);
    void parseUnlisten();
    void receivedAuth(const QJsonObject &, const PeerInfo &, QDtls *);
    void receivedForceLogout(Uid uid);
    void receivedLogin(const QJsonObject &, const PeerInfo &, QDtls *);
    void receivedLogout(const QJsonObject &, const PeerInfo &, QDtls *);
    void receivedReg(const QJsonObject &, const PeerInfo &, QDtls *);
    void receivedReq(const QJsonObject &, const PeerInfo &, QDtls *);
    void sqlcheckEquip();
    void sqlcheckFacto();
    void sqlcheckUsers();
    void sqlinit();
    void sqlinitEquip();
    void sqlinitFacto();
    void sqlinitUsers();

    bool listening = false;
    QUdpSocket serverSocket;

    QSslConfiguration serverConfiguration;
    QDtlsClientVerifier cookieSender;
    std::vector<std::unique_ptr<QDtls>> knownClients;

    QMap<PeerInfo, Uid> connectedUsers;
    QMap<Uid, PeerInfo> connectedPeers;

    QMap<int, QPointer<EquipDef>> equipRegistry;

    std::random_device random;
    std::mt19937 mt;
    static constexpr float dOF = 1.0; // degree of freedom
    std::chi_squared_distribution<float> chi2Dist{dOF};

    const QByteArray defaultSalt =
            QByteArrayLiteral("\xe8\xbf\x99\xe6\x98\xaf\xe4\xb8"
                              "\x80\xe6\x9d\xa1\xe5\x92\xb8\xe9"
                              "\xb1\xbc");
    Q_DISABLE_COPY(Server)
};

#endif // SERVER_H
