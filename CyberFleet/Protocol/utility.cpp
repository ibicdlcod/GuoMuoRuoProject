#include "utility.h"

void Utility::titleCase(QString &input)
{
    input = input.toLower();
    if(input.length() > 0) {
        input.replace(0, 1, input[0].toUpper());
    }
}
