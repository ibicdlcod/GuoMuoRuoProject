#include "ecma48.h"

Ecma48::Ecma48(quint8 red,
               quint8 green,
               quint8 blue,
               bool background)
    : red(red), green(green), blue(blue), background(background)
{

}

Ecma::Ecma(EcmaSetter input)
    : mem1({structInt::ecmaSetter, input})
{

}

Ecma::Ecma(quint8 red,
           quint8 green,
           quint8 blue,
           bool background)
    : mem2({structInt::ecma48, Ecma48(red, green, blue, background)})
{

}

Ecma::Ecma(QTextStreamManipulator manipulator)
    : mem3({structInt::qtextStreamManipulator, manipulator})
{

}

Ecma::~Ecma()
{

}
