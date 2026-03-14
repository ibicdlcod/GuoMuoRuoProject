/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef PEERINFO_H
#define PEERINFO_H

#include <QHostAddress>

struct PeerInfo {
    explicit PeerInfo();
    explicit PeerInfo(const QHostAddress &, quint16);
    bool operator<(const PeerInfo &) const;
    bool operator==(const PeerInfo &) const;
    const QString toString() const;

    QHostAddress address;
    quint16 port;
};

#endif // PEERINFO_H
