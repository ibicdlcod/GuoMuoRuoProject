#include "mapnode.h"
#include <QJsonArray>

MapNode::MapNode(double x, double y, int lbDistance,
                 KP::BattleType type, QList<int> &&nextNodes)
    :x(x), y(y), lbDistance(lbDistance), type(type), nextNodes(nextNodes)
{

}

MapNode::MapNode(const QJsonObject &input)
    :x(input["x"].toDouble()),
    y(input["y"].toDouble()),
    lbDistance(input["lb"].toInt()),
    type(static_cast<KP::BattleType>(input["battletype"].toInt()))
{
    QJsonArray nextNodesInput = input["next"].toArray();
    for(const auto &node: nextNodesInput) {
        nextNodes.append(node.toInt());
    }
}
