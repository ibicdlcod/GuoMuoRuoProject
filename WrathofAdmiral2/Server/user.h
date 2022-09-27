#ifndef USER_H
#define USER_H

#include <QDateTime>
#include <QObject>
#include "peerinfo.h"
#include "resord.h"

namespace User {
    ResOrd getCurrentResources(int uid);
    const QString getName(int uid);
    QDateTime getThrottleTime(int uid);
    void incrementThrottleCount(int uid);
    void naturalRegen(int uid);
    void refreshPort(int uid);
    void removeThrottleCount(int uid);
    void setResources(int uid, ResOrd goal);
    void updateThrottleTime(int uid);
};

#endif // USER_H
