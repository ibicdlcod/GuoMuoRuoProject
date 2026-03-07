#ifndef MAPNODE_H
#define MAPNODE_H

#include "kp.h"

class MapNode
{
public:
    explicit MapNode(double x, double y, int lbDistance,
                     KP::BattleType type, QList<int> &&nextNodes);
    explicit MapNode(const QJsonObject &);
    MapNode() = default;

    double x;
    double y;
    int lbDistance;
    KP::BattleType type;
    QList<int> nextNodes;
};

#endif // MAPNODE_H
