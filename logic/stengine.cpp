#include <QRegularExpression>

#include "stengine.h"

STEngine::STEngine(QObject *parent) : QObject(parent), state(STState::homeport)
{

}

bool STEngine::parse(QString command)
{
    QStringList commandParts = command.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    if(commandParts.length() > 0)
    {
        QString primary = commandParts[0];

        // aliases
        QMap<QString, QString> aliases;
        aliases["ls"] = "showValid";
        aliases["la"] = "showAll";

        if(aliases.contains(primary))
        {
            primary = aliases[primary];
        }
        // end aliases

        for(STState s : STState::_values())
        {
            if(primary.compare(s._to_string(), Qt::CaseInsensitive) == 0)
            {
                state = s;
                return true;
            }
        }
        for(STCType c : STCType::_values())
        {
            if(primary.compare(c._to_string(), Qt::CaseInsensitive) == 0)
            {
                switch (c) {
                case STCType::help:
                    commandParts.removeFirst();
                    emit showHelp(commandParts); break;
                case STCType::showAll:
                    emit showAllCommands(); break;
                case STCType::showValid:
                    emit showCommands(STRelations::validCommands(state)); break;
                default: emit invalidCommand(); return false;
                }
                return true;
            }
        }
    }
    emit invalidCommand();
    return false;
}

QString STEngine::getState()
{
    return state._to_string();
}
