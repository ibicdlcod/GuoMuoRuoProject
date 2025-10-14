#ifndef MAP_H
#define MAP_H

#include "qpoint.h"
#include <QHash>

class Map
{

public:
    Map(int id, int x, int y);

    QString toString(QString lang);

    int id;
    QHash<QString, QString> localNames;
    int x;
    int y;
};

#endif // MAP_H
