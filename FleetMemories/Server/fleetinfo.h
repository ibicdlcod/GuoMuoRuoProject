#ifndef FLEETINFO_H
#define FLEETINFO_H
#include <QHash>
#include <QList>
#include "lua.h"
#include "../Protocol/shipdynamic.h"
#include "../Protocol/kp.h"
#include "../Protocol/ship.h"
#include "../Protocol/equipment.h"
#include "steam/steamclientpublic.h"

class FleetInfo
{
public:
    FleetInfo();
    ~FleetInfo();
    double los();
    LuaMap capitalness();//"Total"/"Surface"/"Carrier"/"Screens"
    std::vector<int> shipSpeeds();

    /* Returns the sum of:
     *   a – ship base attrs scaled by efficiency at current level/star
     *   b – equipment attr contributions scaled by skill-point effect
     *   c – virtual-equipment bonus (zero for now)
     */
    LuaMap effectiveAttr(const CSteamID &uid, int fleetPosIndex);

    Equipment * getEquipAtPosAndSlot(int fleetPosIndex, int slot);
    Equipment * getEquipAtPosAndEXSlot(int fleetPosIndex);
    QList<Equipment *> getAllEquipAtPos(int fleetPosIndex);
    int getPlaneCountAtPosAndSlot(int fleetPosIndex, int slot);
    void setPlaneCountAtPosAndSlot(int fleetPosIndex, int slot, int count);
    void setHPAtPos(int fleetPosIndex, int hp);

    /* Returns the per-ship equipment grid suitable for Lua branch-rule calls. */
    std::vector<std::vector<Equipment *>> getEquipGrid() const;

    KP::FleetType type;
    std::vector<Ship *> ships;
    std::vector<ShipDynamic *> shipDynamics;
    std::vector<int> shipTags; /* a vector of 0 for now */

    /* UUID-keyed equipment lookup; populated by Server::queryFleetInfo. */
    QHash<QUuid, Equipment *> equipMap;
    /* Per-UUID skill-point effect factor; populated by Server::queryFleetInfo. */
    QHash<QUuid, double> equipSkillEffects;
};

#endif // FLEETINFO_H
