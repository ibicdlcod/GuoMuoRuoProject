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
#ifndef DTLSCLIENT_H
#define DTLSCLIENT_H

#include <QtNetwork>
#include <QtCore>

QT_BEGIN_NAMESPACE

//! [0]
class DtlsClient : public QObject
{
    Q_OBJECT

public:
    DtlsClient(const QHostAddress &address, quint16 port,
                    const QString &connectionName);
    ~DtlsClient();
    void startHandshake();

    /* formerly slots */
    static void errorMessage(const QString &message);
    void warningMessage(const QString &message);
    void infoMessage(const QString &message);
    void serverResponse(const QString &clientInfo, const QByteArray &datagraam,
                        const QByteArray &plainText);

public slots:
    void parse(const QString &cmdline);

signals:
    void finished();

private slots:
    void udpSocketConnected();
    void readyRead();
    void handshakeTimeout();
    void pskRequired(QSslPreSharedKeyAuthenticator *auth);
    void pingTimeout();

    /* See https://web.archive.org/web/20201023062748/https://wikiwiki.jp/kanc
     * olle/%E8%89%A6%E5%A8%98%E8%A9%B3%E7%B4%B0%E3%83%86%E3%83%B3%E3%83%97%E3
     * %83%AC
	 * https://wikiwiki.jp/kancolle/%E4%BF%97%E7%A7%B0%E3%83%BB%E3%82%B9%E3%83
	 * %A9%E3%83%B3%E3%82%B0%E9%9B%86/%E3%81%AA#cat
	 * for why this is named cat. */
    void catbomb();

private:
    QString name;
    QUdpSocket socket;
    QDtls crypto;

    QTimer pingTimer;
    unsigned ping = 0;

    unsigned const int maxretransmit;
    unsigned int retransmit_times = 0;

    bool login_success;

    Q_DISABLE_COPY(DtlsClient)
};
//! [0]

QT_END_NAMESPACE

#endif // DTLSCLIENT_H
