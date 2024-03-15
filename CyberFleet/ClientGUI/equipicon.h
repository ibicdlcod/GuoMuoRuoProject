#ifndef EQUIPICON_H
#define EQUIPICON_H

#include "../Protocol/equiptype.h"
#include <QIcon>

namespace EquipIcon
{
/* this function is deliberately not placed in Protocol,
 * otherwith both Protocol and Server must depend on Qt::Gui
 */
QIcon equipIcon(EquipType);
};

#endif // EQUIPICON_H
