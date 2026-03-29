/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef RESEXOTIC_H
#define RESEXOTIC_H

/* 3-Resources.md#Exotic types */
struct ResExotic
{
    ResExotic() = default;
    constexpr ResExotic(int ard, int medal, double sanity = 0.0)
        : ard(ard), medal(medal), sanity(sanity) {}

    constexpr ResExotic& operator+=(const ResExotic& amount) {
        ard    += amount.ard;
        medal  += amount.medal;
        sanity += amount.sanity;
        return *this;
    }
    constexpr ResExotic& operator-=(const ResExotic& amount) {
        ard    -= amount.ard;
        medal  -= amount.medal;
        sanity -= amount.sanity;
        return *this;
    }

    bool spendResources(const ResExotic &);
    bool sufficient() const;

    int    ard    = 0;   // ARD coupon
    int    medal  = 0;   // medal
    double sanity = 0.0; // sanity
};

#endif // RESEXOTIC_H
