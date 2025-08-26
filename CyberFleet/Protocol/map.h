#ifndef MAP_H
#define MAP_H

#include <QObject>

class Map: public QObject
{
    Q_OBJECT

public:
    Map() = delete;
    enum Difficulty {
        Early,
        Medium,
        Late,
        Historical
    };
    Q_ENUM(Difficulty)

    int id;
    int x;
    int y;
};

#endif // MAP_H
