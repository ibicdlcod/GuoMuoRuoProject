#ifndef MAPWITHDIFF_H
#define MAPWITHDIFF_H

#include "map.h"
#include "kp.h"

class MapWithDiff : public Map
{

public:
    explicit MapWithDiff(const Map &Map, KP::Difficulty diff);
    bool operator==(const MapWithDiff &other);

    KP::Difficulty diff;
};

#endif // MAPWITHDIFF_H
