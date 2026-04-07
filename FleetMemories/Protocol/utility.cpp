/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "utility.h"

#include <QQueue>

#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>
#include <vector>

#include "kp.h"

void Utility::titleCase(QString &input)
{
    input = input.toLower();
    if(input.length() > 0) {
        input.replace(0, 1, input[0].toUpper());
    }
}

bool Utility::checkMask(qint32 input, qint32 mask, qint32 desired)
{
    /* You should perceive input as 32-bit binary data */
    return (input & mask) == desired;
}

QHash<int, QSet<int>> Utility::buildSupplyAdjacency(
    const QList<QPair<int,int>> &edges,
    const QMap<int, double> &supremacies)
{
    QHash<int, QSet<int>> adj;
    for(const auto &[a, b] : edges) {
        if(supremacies.value(a, -1.0) >= 0.0
           && supremacies.value(b, -1.0) >= 0.0) {
            adj[a].insert(b);
            adj[b].insert(a);
        }
    }
    return adj;
}

std::tuple<bool, bool, double> Utility::computeAttrition(
    const QHash<int, QSet<int>> &adj,
    const QMap<int, double> &supremacies,
    int targetMap,
    double expectedSupremacy)
{
    /* All designated home ports (5.2-homeport.md) */
    static const KP::AllegianceGroup allNations[] = {
        KP::Japanese, KP::German,  KP::Italian,
        KP::American, KP::British, KP::French,
        KP::Soviet,   KP::Commonwealth
    };

    constexpr double kInf =
        std::numeric_limits<double>::infinity();
    using PD = std::pair<double, int>;

    /* Collect unlocked home port seeds */
    QSet<int> seeds;
    for(KP::AllegianceGroup nation : allNations) {
        int homeId = KP::homePortMap(nation);
        if(homeId != 0
           && supremacies.value(homeId, -1.0) >= 0.0) {
            seeds.insert(homeId);
        }
    }
    if(seeds.isEmpty()) {
        return {false, false, 0.0};
    }

    /* Pass 1: BFS — graph reachability ignoring costs */
    QSet<int> visited;
    QQueue<int> bfsQ;
    for(int s : seeds) {
        if(!visited.contains(s)) {
            visited.insert(s);
            bfsQ.enqueue(s);
        }
    }
    while(!bfsQ.isEmpty()) {
        int u = bfsQ.dequeue();
        for(int v : adj.value(u)) {
            if(!visited.contains(v)) {
                visited.insert(v);
                bfsQ.enqueue(v);
            }
        }
    }
    if(!visited.contains(targetMap)) {
        return {false, false, 0.0};
    }

    /* Pass 2: Dijkstra — minimum log-sum cost path.
     * Cost is charged on departure from u, so the home
     * port's supremacy is included and the target's is not.
     * 0-supremacy nodes (infinite departure cost) are
     * skipped; if all routes require them the target will
     * not appear in dist → finiteRoute = false. */
    std::priority_queue<PD, std::vector<PD>,
                        std::greater<PD>> pq;
    QHash<int, double> dist;
    for(int s : seeds) {
        dist[s] = 0.0;
        pq.push({0.0, s});
    }
    while(!pq.empty()) {
        auto [d, u] = pq.top();
        pq.pop();
        if(d > dist.value(u, kInf)) {
            continue;
        }
        if(u == targetMap) {
            break;
        }
        double sup_u = supremacies.value(u, 0.0);
        double si_u =
            std::min(1.0, sup_u / expectedSupremacy);
        if(si_u <= 0.0) {
            continue;
        }
        double depart = -std::log(si_u);
        for(int v : adj.value(u)) {
            double nd = d + depart;
            if(nd < dist.value(v, kInf)) {
                dist[v] = nd;
                pq.push({nd, v});
            }
        }
    }

    if(!dist.contains(targetMap)) {
        return {true, false, 0.0};
    }
    return {true, true, std::exp(dist[targetMap]) - 1.0};
}
