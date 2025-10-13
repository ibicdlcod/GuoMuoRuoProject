#include "mapwithdiff.h"

MapWithDiff::MapWithDiff(const Map &map, KP::Difficulty diff)
    : Map{map}, diff(diff)
{

}

bool MapWithDiff::operator==(const MapWithDiff &other) {
    return this->id == other.id && this->diff == other.diff;
}
