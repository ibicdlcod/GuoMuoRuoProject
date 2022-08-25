#ifndef NETWORKERROR_H
#define NETWORKERROR_H

#include <stdexcept>
#include <QString>
#include <QCoreApplication>

class NetworkError : public std::runtime_error {
    Q_DECLARE_TR_FUNCTIONS(NetworkError)

public:
    NetworkError(QString);
};

#endif // NETWORKERROR_H
