/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef USER_H
#define USER_H

#include <QDateTime>
#include <QObject>
#include "../steam/steamclientpublic.h"
#include "../Protocol/equipment.h"
#include "../Protocol/resord.h"
#include "server.h"

namespace User {

bool addShipBP(const CSteamID &, int, bool reverse = false);
void addSkillPoints(const CSteamID &, int, int64);
KP::ShipNationality checkHomePort(const CSteamID &);
void decideHomePort(const CSteamID &, KP::ShipNationality);
int getCurrentFactoryParallel(const CSteamID &, int);
ResOrd getCurrentResources(const CSteamID &);
int getEquipAmount(const CSteamID &, int);
int getEquipDef(QUuid);
int getShipDef(QUuid);
int64 getSkillPoints(const CSteamID &, int);
std::pair<bool, int> haveFather(const CSteamID &,
                                int, QMap<int, Equipment *> &);
std::tuple<bool, int, int64> haveMotherSP(const CSteamID &,
                                           int, QMap<int, Equipment *> &,
                                           int64);
Q_DECL_DEPRECATED void init(const CSteamID &);
bool isFactoryBusy(const CSteamID &, int);
std::tuple<bool, int> isFactoryFinished(const CSteamID &, int);
bool isSuperUser(const CSteamID &);
QUuid newEquip(const CSteamID &, int);
QUuid newShip(const CSteamID &, int, int);
bool openMap(const CSteamID &, int);
void refreshFactory(Server *server, const CSteamID &);
void refreshPort(Server *server, const CSteamID &);
void setResources(const CSteamID &, ResOrd);

}

#endif // USER_H
