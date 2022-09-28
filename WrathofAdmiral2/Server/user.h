#ifndef USER_H
#define USER_H

#include <QDateTime>
#include <QObject>
#include "equipment.h"
#include "peerinfo.h"
#include "resord.h"

namespace User {

ResOrd getCurrentResources(int uid);
const QString getName(int uid);
QDateTime getThrottleTime(int uid);
void incrementThrottleCount(int uid);
void init(int uid);
bool isFactoryBusy(int uid, int factoryID);
std::tuple<bool, int> isFactoryFinished(int uid, int factoryID);
void naturalRegen(int uid);
void refreshFactory(int uid);
void refreshPort(int uid);
void removeThrottleCount(int uid);
void setResources(int uid, ResOrd goal);
void updateThrottleTime(int uid);

};

#endif // USER_H
