/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "resexotic.h"

/* attempt to spend resources, will not change if failed */
bool ResExotic::spendResources(const ResExotic &amount) {
    operator-=(amount);
    if(!sufficient()){
        operator+=(amount);
        return false;
    }
    else {
        return true;
    }
}

bool ResExotic::sufficient() const {
    return !(ard < 0 || medal < 0 || sanity < 0.0);
}
