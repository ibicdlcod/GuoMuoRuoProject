#ifndef USER_H
#define USER_H

#include <QDateTime>
#include <QObject>
#include "../steam/steamclientpublic.h"
#include "../Protocol/equipment.h"
#include "../Protocol/resord.h"

namespace User {

void addSkillPoints(CSteamID &, int, uint64);
ResOrd getCurrentResources(CSteamID &);
std::pair<bool, int> haveFather(CSteamID &, int, QMap<int, Equipment *> &);
void init(CSteamID &);
bool isFactoryBusy(CSteamID &, int);
std::tuple<bool, int> isFactoryFinished(CSteamID &, int);
bool isSuperUser(CSteamID &);
void naturalRegen(CSteamID &);
int newEquip(CSteamID &, int);
void refreshFactory(CSteamID &);
void refreshPort(CSteamID &);
void setResources(CSteamID &, ResOrd);
};

#endif // USER_H
