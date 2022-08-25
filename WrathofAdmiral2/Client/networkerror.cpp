#include "networkerror.h"

NetworkError::NetworkError(QString what)
//% "Network Error: %1"
    : std::runtime_error(qtTrId("network-error").arg(what).toStdString()) {

}
