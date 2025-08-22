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

#ifndef CLIENTV2_H
#define CLIENTV2_H

#include <QColor>
#include <QtNetwork>
#include "../Protocol/equipment.h"
#include "../Protocol/receiver.h"
#include "../Protocol/sender.h"
#include "../Protocol/kp.h"
#include "steamauth.h"
#include "ui/developwindow.h"
#include "ui/techview.h"
#include "equipmodel.h"

void customMessageHandler(QtMsgType,
                          const QMessageLogContext &,
                          const QString &);


class Clientv2 : public QObject {
    Q_OBJECT

public:
    virtual ~Clientv2() noexcept;

    static Clientv2 & getInstance() {
        static Clientv2 instance;
        return instance;
    }

    enum Password{
        normal,
        login,
        registering,
        confirm
    };
    Q_ENUM(Password);
    bool isEquipRegistryCacheGood() const;
    bool loggedIn() const;
    void doFetch(const QStringList &);

    /* ususally accesses equipregistryCache */
    friend int DevelopWindow::equipIdDesired();
    friend void DevelopWindow::resetListName(int);
    friend void TechView::demandLocalTech(int);
    friend void TechView::demandSkillPoints(int);
    friend void TechView::resetLocalListName();
    friend void DevelopWindow::displaySuccessRate2();

    int equipBigTypeIndex = 0;
    int equipIndex = 0;
    EquipModel equipModel;
    QMap<int, double> techCache;

public slots:
    void autoPassword();
    void backToNavalBase();
    void catbomb();
    void demandEquipCache();
    void displayPrompt();
    void doDestructEquip(const QList<QUuid> &);
    void doRefreshFactory();
    void doRefreshFactoryArsenal();
    Equipment * getEquipmentReg(int);
    bool parse(const QString &);
    void parseDisconnectReq();
    void parseQuit();
    bool parseSpec(const QStringList &);
    void sendEncryptedAppTicket(uint8 [], uint32);
    void serverResponse(const QString &, const QByteArray &);
    void serverResponseStd(const QJsonObject &);
    void serverResponseNonStd(const QByteArray &);
    void setTicketCache(uint8 [], uint32);
    void showHelp(const QStringList &);
    void switchToFactory();
    void switchToTech();
    void switchToTech2();
    void uiRefresh();
    Q_DECL_DEPRECATED void update();

signals:
    void aboutToQuit();
    void equipRegistryComplete();
    void gamestateChanged(KP::GameState);
    void qout(QString, QColor background = QColor("white"),
              QColor foreground = QColor("black"));
    void receivedArsenalEquip(const QJsonObject &);
    void receivedFactoryRefresh(const QJsonObject &);
    void receivedGlobalTechInfo(const QJsonObject &);
    void receivedGlobalTechInfo2(const QJsonObject &);
    void receivedLocalTechInfo(const QJsonObject &);
    void receivedLocalTechInfo2(const QJsonObject &);
    void receivedResourceInfo(const QJsonObject &);
    void receivedSkillPointInfo(const QJsonObject &);

private slots:
    void changeGameState(KP::GameState);
    void encrypted();
    void errorOccurred(QAbstractSocket::SocketError);
    void errorOccurredStr(const QString &);
    void handshakeInterrupted(const QSslError &);
    void pskRequired(QSslPreSharedKeyAuthenticator *);
    void readyRead();
    void sendEATActual();
    void shutdown();

private:
    void doAddEquip(const QStringList &);
    void doDevelop(const QStringList &);
    void doDeleteTestEquip();
    void doGenerateTestEquip();
    void doSwitch(const QStringList &);
    void exitGracefully();
    void exitGraceSpec();
    QString gameStateString() const;
    static const QStringList getCommands();
    const QStringList getCommandsSpec() const;
    int getConsoleWidth();
    const QStringList getValidCommands() const;
    void invalidCommand();
    bool loginCheck();
    void parseConnectReq(const QStringList &);
    bool parseGameCommands(const QString &, const QStringList &);
    void qls(const QStringList &);
    void readWhenConnected(const QByteArray &);
    void readWhenUnConnected(const QByteArray &);
    void receivedAuth(const QJsonObject &);
    void receivedInfo(const QJsonObject &);
    void receivedLogin(const QJsonObject &);
    void receivedLogout(const QJsonObject &);
    void receivedMsg(const QJsonObject &);
    void receivedNewLogin(const QJsonObject &);
    void sendTestMessages();
    void showCommands(bool);
    void switchCert(const QStringList &);
    void updateEquipCache(const QJsonObject &);

    explicit Clientv2(QObject * parent = nullptr);

    QHostAddress address;
    quint16 port;

    QSslSocket socket;
    QSslConfiguration conf;
    Receiver recv;
    QPointer<Sender> sender;

    unsigned int maxRetransmit;
    unsigned int retransmitTimes = 0;

    QString clientName;
    QString serverName;

    bool attemptMode;
    bool logoutPending;

    KP::GameState gameState;
    QMap<int, Equipment *> equipRegistryCache;
    bool equipRegistryCacheGood = false;

#pragma message(SALT_FISH)
    const QByteArray defaultSalt =
        QByteArrayLiteral("\xe8\xbf\x99\xe6\x98\xaf\xe4\xb8"
                          "\x80\xe6\x9d\xa1\xe5\x92\xb8\xe9"
                          "\xb1\xbc");

    SteamAuth sauth;
    QByteArray authCache;
    bool authSent = false;

    QTimer *timer;

    Q_DISABLE_COPY_MOVE(Clientv2)
};

#endif // CLIENTV2_H
