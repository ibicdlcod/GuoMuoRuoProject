/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "networkerror.h"

NetworkError::NetworkError(QString what)
    : std::runtime_error(qtTrId("network-error").arg(what).toStdString()) {

}
