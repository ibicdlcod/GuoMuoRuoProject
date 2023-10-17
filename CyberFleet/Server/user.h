#ifndef USER_H
#define USER_H

#include <QDateTime>
#include <QObject>
#include "../steam/steamclientpublic.h"
#include "../Protocol/equipment.h"
#include "../Protocol/resord.h"
#include "peerinfo.h"

namespace User {

ResOrd getCurrentResources(CSteamID &uid);
void init(CSteamID &uid);
bool isFactoryBusy(CSteamID &uid, int factoryID);
std::tuple<bool, int> isFactoryFinished(CSteamID &uid, int factoryID);
bool isSuperUser(CSteamID &uid);
void naturalRegen(CSteamID &uid);
int newEquip(CSteamID &uid, int equipDid);
void refreshFactory(CSteamID &uid);
void refreshPort(CSteamID &uid);
void setResources(CSteamID &uid, ResOrd goal);

};

#endif // USER_H
