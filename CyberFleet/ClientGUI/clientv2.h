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
#include "commandline.h"
#include "steamauth.h"

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
    bool loggedIn() const;
    void sendEncryptedAppTicket(uint8 [], uint32);

public slots:
    void autoPassword();
    void backToNavalBase();
    void catbomb();
    void displayPrompt();
    bool parse(const QString &);
    void parseDisconnectReq();
    void parseQuit();
    bool parseSpec(const QStringList &);
    void serverResponse(const QString &, const QByteArray &);
    void showHelp(const QStringList &);
    void switchToFactory();
    void uiRefresh();
    Q_DECL_DEPRECATED void update();

signals:
    void aboutToQuit();
    void gamestateChanged(KP::GameState);
    void qout(QString, QColor background = QColor("white"),
              QColor foreground = QColor("black"));
    void receivedFactoryRefresh(const QJsonObject &);
    //void sentAppTicket();

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
    void doRefreshFactory();
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
    void showCommands(bool);

    explicit Clientv2(QObject * parent = nullptr);

    QHostAddress address;
    quint16 port;

    QSslSocket socket;
    QSslConfiguration conf;

    unsigned int maxRetransmit;
    unsigned int retransmitTimes = 0;

    QString clientName;
    QString serverName;

    bool attemptMode;
    bool logoutPending;

    KP::GameState gameState;

    static const unsigned int defaultMaxRetransmit = 2;
    const QByteArray defaultSalt =
            QByteArrayLiteral("\xe8\xbf\x99\xe6\x98\xaf\xe4\xb8"
                              "\x80\xe6\x9d\xa1\xe5\x92\xb8\xe9"
                              "\xb1\xbc");

    SteamAuth sauth;

    QTimer *timer;

    Q_DISABLE_COPY(Clientv2)
};

#endif // CLIENTV2_H
