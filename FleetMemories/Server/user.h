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
KP::AllegianceGroup checkHomePort(const CSteamID &);
double checkMapSupremacy(const CSteamID &uid, int);
void decideHomePort(const CSteamID &, KP::AllegianceGroup);
bool decreaseGauge(const CSteamID &, int, KP::Difficulty, int);
int getCurrentFactoryParallel(const CSteamID &, int);
int getCurrentMapOpened(const CSteamID &);
ResOrd getCurrentResources(const CSteamID &);
/* factoryslot, repairslot */
std::tuple<int, int> getCurrentSlots(const CSteamID &);
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
bool isDockBusy(const CSteamID &, int);
bool isFactoryBusy(const CSteamID &, int);
std::tuple<bool, int> isFactoryFinished(const CSteamID &, int);
bool isGaugeFinished(const CSteamID &, int, KP::Difficulty);
bool isMapUnlocked(const CSteamID &, int, KP::Difficulty);
bool isSuperUser(const CSteamID &);
QUuid newEquip(const CSteamID &, int);
QUuid newShip(const CSteamID &, int, int);
bool openMap(const CSteamID &, int, int gauge = 0);
void refreshFactory(Server *server, const CSteamID &);
void refreshPort(Server *server, const CSteamID &);
bool setMapSupremacy(const CSteamID &, int, double, double);
void setResources(const CSteamID &, ResOrd);

}

#endif // USER_H
