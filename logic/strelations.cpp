#include <QMultiMap>
#include <list>

#include "strelations.h"

namespace STRelations {

QMultiMap<STState, STCType> validRelations;

STCType commons[] = {STCType::help,
                     STCType::showValid,
                     STCType::showAll
                    };

std::list<STCType> commons2(std::begin(commons), std::end(commons));

void initRelations()
{
    for(auto const &a: commons2)
    {
        for(STState b: STState::_values())
        {
            validRelations.insert(b, a);
        }
    }
}

QList<STState> validStates(STCType type)
{
    QList<STState> result;
    for(STState b: STState::_values())
    {
        if(validRelations.contains(b, type))
        {
            result.append(b);
        }
    }
    return result;
}

const QList<STCType> validCommands(STState state)
{
    return validRelations.values(state);
}

}
