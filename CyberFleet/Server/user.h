#ifndef USER_H
#define USER_H

#include <QDateTime>
#include <QObject>
#include "../steam/steamclientpublic.h"
#include "../Protocol/equipment.h"
#include "../Protocol/resord.h"

namespace User {

void addSkillPoints(const CSteamID &, int, uint64);
ResOrd getCurrentResources(const CSteamID &);
uint64 getSkillPoints(const CSteamID &, int);
std::pair<bool, int> haveFather(const CSteamID &, int, QMap<int, Equipment *> &);
void init(const CSteamID &);
bool isFactoryBusy(const CSteamID &, int);
std::tuple<bool, int> isFactoryFinished(const CSteamID &, int);
bool isSuperUser(const CSteamID &);
void naturalRegen(const CSteamID &);
int newEquip(const CSteamID &, int);
void refreshFactory(const CSteamID &);
void refreshPort(const CSteamID &);
void setResources(const CSteamID &, ResOrd);
};

#endif // USER_H
