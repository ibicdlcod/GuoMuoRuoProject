#ifndef STRELATIONS_H
#define STRELATIONS_H

/* otherwise the constructor of the better_enum will be private */
#define BETTER_ENUMS_DEFAULT_CONSTRUCTOR(Enum) \
  public:                                      \
    Enum() = default;

#include <QList>
#include <enum.h>

BETTER_ENUM(STCType, quint8,
            help,
            showValid,
            showAll
)

BETTER_ENUM(STState, quint8,
    homeport,
    composition,
    supply,
    equipments,
    remodel,
    construction,
    development,
    development2,
    improvementArsenal,

    sortie,
    sortieing,
    practice,
    practicing,
    expedition,

    hqInformation,
    hqBattlerank,
    friendlyfleet,
    collection,
    itemOwned,
    itemBought,
    itemShop,
    furnitureShop,
    furnitureChange,

    quest
)

namespace STRelations {

    void initRelations();
    QList<STState> validStates(STCType type);
    const QList<STCType> validCommands(STState state);
}

#endif // STRELATIONS_H
