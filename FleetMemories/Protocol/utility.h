/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef UTILITY_H
#define UTILITY_H

#include <QHash>
#include <QList>
#include <QMap>
#include <QPair>
#include <QSet>
#include <QString>

#include <tuple>

#include "kp.h"

namespace Utility {

void titleCase(QString &input);
bool checkMask(qint32 input, qint32 mask, qint32 desired);

/* 8.1-supply.md#Supply_chain_and_attrition
 * Build undirected supply chain adjacency from edge list,
 * restricted to maps with supremacy >= 0 (unlocked). */
QHash<int, QSet<int>> buildSupplyAdjacency(
    const QList<QPair<int,int>> &edges,
    const QMap<int, double> &supremacies);

/* Multi-source Dijkstra on the node-weighted supply chain.
 * Node cost = max(0, -log(min(1, sup/expectedSupremacy))).
 * Cost charged on departure so home port is included and
 * the target map is excluded from the product.
 * Seeds: all designated home ports that are unlocked.
 *
 * Returns (reachable, finiteRoute, attrition):
 *  reachable=false     → target disconnected from all ports
 *  reachable, !finite  → only reachable via 0-sup nodes
 *  reachable, finite   → attrition = exp(dist) - 1
 */
std::tuple<bool, bool, double> computeAttrition(
    const QHash<int, QSet<int>> &adj,
    const QMap<int, double> &supremacies,
    int targetMap,
    double expectedSupremacy);

} // namespace Utility

#endif // UTILITY_H
