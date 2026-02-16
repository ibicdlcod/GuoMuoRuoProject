/* Copyright (C) 2026 Harusoft Inc.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "peerinfo.h"

PeerInfo::PeerInfo()
    : address(QHostAddress()), port(0) {
}

PeerInfo::PeerInfo(const QHostAddress &address, quint16 port)
    : address(address), port(port) {
}

bool PeerInfo::operator<(const PeerInfo &other) const {
    return address.toString() < other.toString();
}

bool PeerInfo::operator==(const PeerInfo &other) const {
    return address.isEqual(other.address) && port == other.port;
}

const QString PeerInfo::toString() const {
    return QStringLiteral("(%1:%2)").arg(address.toString()).arg(port);
}
