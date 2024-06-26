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
#include <random>
#include "steam/steamclientpublic.h"

#include "../Protocol/commandline.h"
#include "../Protocol/equipment.h"
#include "../Protocol/peerinfo.h"
#include "../Protocol/receiver.h"
#include "../Protocol/ship.h"
#include "servermastersender.h"
#include "sslserver.h"

class Server : public CommandLine {
    Q_OBJECT

public:
    explicit Server(int, char **);
    ~Server() noexcept override;

    void datagramReceived(const PeerInfo &, const QByteArray &, QSslSocket *);
    void datagramReceivedNonStd(const QByteArray &, const PeerInfo &, QSslSocket *);
    void datagramReceivedStd(const QJsonObject &, const PeerInfo &, QSslSocket *);
    bool listen(const QHostAddress &, quint16);

public slots:
    void displayPrompt() override;
    bool parseSpec(const QStringList &) override;
    void readyRead(QSslSocket *);
    Q_DECL_DEPRECATED void update();

private slots:
    void alertReceived(QSslSocket *socket, QSsl::AlertLevel level,
                       QSsl::AlertType type, const QString &description);
    std::pair<double, QList<TechEntry>>
    calculateTech(const CSteamID &, int jobID = 0);
    void handleNewConnection();
    double getSkillPointsEffect(const CSteamID &, int);
    void offerEquipInfo(QSslSocket *, int);
    void offerEquipInfoUser(const CSteamID &, QSslSocket *);
    void offerTechInfo(QSslSocket *, const CSteamID &, int jobID = 0);
    void offerTechInfoComponents(QSslSocket *, const QList<TechEntry> &,
                                   bool, bool);
    void offerResourceInfo(QSslSocket *, const CSteamID &);
    void offerSPInfo(QSslSocket *, const CSteamID &, int);
    void pskRequired(QSslSocket *, QSslPreSharedKeyAuthenticator *);
    void senderMErrorMessage(const QString &);
    void shutdown();
    void sslErrors(QSslSocket *, const QList<QSslError> &);

private:
    bool addEquipStar(const QUuid &, int);
    void decryptDatagram(QSslSocket *, const QByteArray &);
    void deleteTestEquip(const CSteamID &);
    void doDevelop(CSteamID &, int, int, QSslSocket *);
    void doFetch(CSteamID &, int, QSslSocket *);
    void doHandshake(QSslSocket *, const QByteArray &);
    [[nodiscard]] bool equipmentRefresh();
    void exitGraceSpec() override;
    bool exportEquipToCSV() const;
    void generateEquipChilds(int, int);
    void generateTestEquip(const CSteamID &);
    const QStringList getCommandsSpec() const override;
    const QStringList getValidCommands() const override;
    bool importEquipFromCSV();
    bool importShipFromCSV();
    void naturalRegen(const CSteamID &);
    QUuid newEquip(const CSteamID &, int);
    int64 newEquipHasMotherCal(int);
    void newEquipHasMother(const CSteamID &, int);
    void parseListen(const QStringList &);
    void parseUnlisten();
    void receivedAuth(const QJsonObject &, const PeerInfo &, QSslSocket *);
    void receivedForceLogout(CSteamID &);
    void receivedLogin(CSteamID &, const PeerInfo &, QSslSocket *);
    void receivedLogout(CSteamID &, const PeerInfo &, QSslSocket *);
    void receivedReq(const QJsonObject &, const PeerInfo &, QSslSocket *);
    void refreshClientFactory(CSteamID &, QSslSocket *);
    QList<QUuid> retireEquip(const CSteamID &, const QList<QUuid> &);
    void sendTestMessages();
    [[nodiscard]] bool shipRefresh();
    Q_DECL_DEPRECATED void sqlcheckEquip();
    Q_DECL_DEPRECATED void sqlcheckEquipU();
    Q_DECL_DEPRECATED void sqlcheckFacto();
    Q_DECL_DEPRECATED void sqlcheckUsers();
    void sqlinit();
    void sqlinitEquip();
    void sqlinitEquipName();
    void sqlinitEquipSP();
    void sqlinitEquipU();
    void sqlinitFacto();
    void sqlinitShip();
    void sqlinitShipName();
    void sqlinitUsers() const;
    void sqlinitUserA() const;
    void switchCert(const QStringList &);
    void userInit(CSteamID &);

    bool listening = false;
    SslServer sslServer;
    QHash<QSslSocket *, CSteamID> connectedUsers;
    QMap<CSteamID, QSslSocket *> connectedPeers;
    ServerMasterSender senderM;
    Receiver receiverM;

    QSet<int> openEquips;
    QMap<int, Equipment *> equipRegistry;
    QMultiMap<int, int> equipChildTree;

    QSet<int> openShips;
    QMap<int, Ship *> shipRegistry;

    std::random_device random;
    std::mt19937 mt;

    const QByteArray defaultSalt =
            QByteArrayLiteral("\xe8\xbf\x99\xe6\x98\xaf\xe4\xb8"
                              "\x80\xe6\x9d\xa1\xe5\x92\xb8\xe9"
                              "\xb1\xbc");

    Q_DISABLE_COPY(Server)
};

#endif // SERVER_H
