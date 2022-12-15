#include "networkerror.h"

NetworkError::NetworkError(QString what)
    : std::runtime_error(qtTrId("network-error").arg(what).toStdString()) {

}
