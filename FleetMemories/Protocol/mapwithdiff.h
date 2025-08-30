#ifndef MAPWITHDIFF_H
#define MAPWITHDIFF_H

#include "map.h"

class MapWithDiff : public Map
{
    Q_OBJECT

public:
    MapWithDiff(int id, Difficulty diff);
    bool operator==(const MapWithDiff &other);

    Difficulty diff;
};

#endif // MAPWITHDIFF_H
