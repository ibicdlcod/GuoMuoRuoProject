#ifndef ECMA48_H
#define ECMA48_H

#include <QObject>
#include "consoletextstream.h"

enum class EcmaSetter
{
    UnderscoreOn,
    UnderscoreOff,
    BlinkOn,
    BlinkOff,
    ReverseVideo,
    NormalVideo,
    TextDefault,
    BgDefault,
    AllDefault
};


struct Ecma48 : public QObject
{
    Q_OBJECT
public:
    explicit Ecma48(quint8 red = 0,
                    quint8 green = 0,
                    quint8 blue = 0,
                    bool background = false);
    quint8 red;
    quint8 green;
    quint8 blue;
    bool background;

signals:

};

ConsoleTextStream & operator<<(ConsoleTextStream &, EcmaSetter);
ConsoleTextStream & operator<<(ConsoleTextStream &, Ecma48);

#endif // ECMA48_H
