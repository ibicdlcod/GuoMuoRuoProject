/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef RESEXOTIC_H
#define RESEXOTIC_H

/* 3-Resources.md#Exotic types */
struct ResExotic
{
    ResExotic() = default;
    constexpr ResExotic(int ard, int medal) : ard(ard), medal(medal) {}

    constexpr ResExotic& operator+=(const ResExotic& amount) {
        ard   += amount.ard;
        medal += amount.medal;
        return *this;
    }
    constexpr ResExotic& operator-=(const ResExotic& amount) {
        ard   -= amount.ard;
        medal -= amount.medal;
        return *this;
    }

    bool spendResources(const ResExotic &);
    bool sufficient() const;

    int ard   = 0; // ARD coupon
    int medal = 0; // medal
};

#endif // RESEXOTIC_H
