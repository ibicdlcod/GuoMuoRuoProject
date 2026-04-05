/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "server_plane_replenish.h"
#include "server.h"
#include "equipment.h"
#include "user.h"
#include "kerrors.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QTimer>

PlaneReplenish::PlaneReplenish(Server *parent)
    : QObject(parent), server(parent) {}

bool PlaneReplenish::replenishAfterBattle(const CSteamID &uid, int fleetIndex) {
    // Implementation in next steps
    return false;
}

void PlaneReplenish::storePlaneLosses(const CSteamID &uid, const QString &shipUuid,
                                      int slot, int equipDef, int lossCount, int remainingCount) {
    // Implementation in next steps
}

bool PlaneReplenish::recoverPlaneLosses(const CSteamID &uid) {
    // Implementation in next steps
    return false;
}

ResOrd PlaneReplenish::calculateReplenishCost(const CSteamID &uid, int fleetIndex) {
    // Implementation in next steps
    return ResOrd();
}

bool PlaneReplenish::applyReplenishment(const CSteamID &uid, int fleetIndex,
                                        const ResOrd &cost) {
    // Implementation in next steps
    return false;
}