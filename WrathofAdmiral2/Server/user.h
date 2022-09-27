#ifndef USER_H
#define USER_H

#include <QDateTime>
#include <QObject>
#include "peerinfo.h"

namespace User {
    const QString getname(int uid);
    QDateTime getThrottleTime(int uid);
    void incrementThrottleCount(int uid);
    void removeThrottleCount(int uid);
    void updateThrottleTime(int uid);
};

#endif // USER_H
