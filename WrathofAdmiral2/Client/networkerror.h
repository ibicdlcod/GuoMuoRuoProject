#ifndef NETWORKERROR_H
#define NETWORKERROR_H

#include <stdexcept>
#include <QString>
#include <QCoreApplication>

class NetworkError : public std::runtime_error
{
    Q_DECLARE_TR_FUNCTIONS(NetworkError)

public:
    //% "Network Error: %1"
    NetworkError(QString what) : std::runtime_error(qtTrId("network-error").arg(what).toStdString()) {};
};

#endif // NETWORKERROR_H
