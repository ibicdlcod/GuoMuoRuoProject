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
    enum TransportMode {
        Default
    };
    FleetInfo();
    ~FleetInfo();
    double los();
    QMap<KP::CapitalType, int> capitalness();
    std::vector<int> shipSpeeds();
    int transportCapacity(const CSteamID &uid, TransportMode mode = Default);

    /* Returns the sum of:
     *   a – ship base attrs scaled by efficiency at current level/star
     *   b – equipment attr contributions scaled by skill-point effect
     *   c – virtual-equipment bonus (zero for now)
     */
    LuaMap effectiveAttr(const CSteamID &uid, int fleetPosIndex);

    Equipment * getEquipAtPosAndSlot(int fleetPosIndex, int slot);
    Equipment * getEquipAtPosAndEXSlot(int fleetPosIndex);
    QList<Equipment *> getAllEquipAtPos(int fleetPosIndex) const;
    /* Returns headquarters equipment ID (see KP::headquartersEquip* constants) if present on ship at
     * position 1, otherwise 0. Checks fleet‑type restrictions per spec. */
    int headquartersEquipId(bool isExpedition) const;
    /* Returns list of fleet positions (0‑based indices) that can escort critically
     * damaged ships. A ship is eligible if it is a destroyer or light cruiser,
     * is healthy (HP > 0, not fled), and the fleet possesses a headquarters
     * equipment (see headquartersEquipId). */
    QList<int> findEscortCandidates(bool isExpedition) const;
    /* Attempts to perform an escorted retreat for the critically damaged ship at
     * damagedPos (0‑based). Returns true if retreat succeeded (ship was critically
     * damaged, at least one escort candidate exists, and both ships marked as fled).
     * If the ship is not critically damaged or no candidates exist, returns false. */
    bool performEscortRetreat(int damagedPos, bool isExpedition);
    int getPlaneCountAtPosAndSlot(int fleetPosIndex, int slot);
    void setPlaneCountAtPosAndSlot(int fleetPosIndex, int slot, int count);
    void setHPAtPos(int fleetPosIndex, int hp);

    /* Returns the per-ship equipment grid suitable for Lua branch-rule calls. */
    std::vector<std::vector<Equipment *>> getEquipGrid() const;

    /* effectiveAttr helpers – exposed as statics for external testing. */
    static LuaMap attrFromShip(const Ship *ship, const ShipDynamic *dyn);
    static LuaMap attrFromEquipment(const Ship *ship, const ShipDynamic *dyn,
                                    const QHash<QUuid, Equipment *> &equipMap,
                                    const QHash<QUuid, double> &skillEffects);

    /* Returns the first-type visible bonus multiplier for one equip slot.
     * Always 1.0 for now; future implementations may depend on ship
     * definition, dynamic state, and slot position. */
    static double getVisibleBonusFirstType(const Ship *ship,
                                           const ShipDynamic *dyn,
                                           int equipPos);

    /* Returns the second-type (virtual-equipment) visible bonus addend.
     * Always an empty map for now; future implementations may add
     * bonus attributes from virtual equipment. */
    static LuaMap getVisibleBonusSecondType(const Ship *ship,
                                            const ShipDynamic *dyn);

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
