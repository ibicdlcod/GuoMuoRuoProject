#ifndef ECMA48_H
#define ECMA48_H

#include <QTextStreamManipulator>

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
    AllDefault,
    ItalicsOn,
    ItalicsOff,
    DoubleScore,
    Invisible,
    Visible,
    CrossedOut,
    NotCrossedOut,
    OverlineOn,
    OverlineOff
};

struct Ecma48
{
public:
    explicit Ecma48(quint8 red,
                    quint8 green,
                    quint8 blue,
                    bool background = false);
    quint8 red;
    quint8 green;
    quint8 blue;
    bool background;
};

enum structInt
{
    ecmaSetter,
    ecma48,
    qtextStreamManipulator
};

union Ecma
{
    structInt member;
    struct
    {
        structInt structId;
        EcmaSetter setter;
    } mem1;
    struct
    {
        structInt structId;
        Ecma48 color;
    } mem2;
    struct
    {
        structInt structId;
        QTextStreamManipulator manipulator;
    } mem3;
    Ecma(QTextStreamManipulator);
    Ecma(EcmaSetter);
    Ecma(quint8 red,
         quint8 green,
         quint8 blue,
         bool background = false);
    ~Ecma();
};

#endif // ECMA48_H
