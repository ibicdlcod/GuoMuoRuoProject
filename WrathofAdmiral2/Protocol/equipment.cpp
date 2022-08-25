#include "equipment.h"

Equipment::Equipment(int id,
                     QString &&name,
                     QString &&type,
                     QMap<AttrType, int> &&attr,
                     QStringList &&customflags)
    : id(id),
      name(name),
      type(type),
      attr(attr),
      customflags(customflags) {
}
