#include "utility.h"

void Utility::titleCase(QString &input)
{
    input = input.toLower();
    if(input.length() > 0) {
        input.replace(0, 1, input[0].toUpper());
    }
}

bool Utility::checkMask(qint32 input, qint32 mask, qint32 desired)
{
    /* You should perceive input as 32-bit binary data */
    return (input & mask) == desired;
}
