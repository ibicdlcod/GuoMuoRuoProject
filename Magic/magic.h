#ifndef MAGIC_H
#define MAGIC_H

#define STRING2(x) #x
#define STRING(x) STRING2(x)
#define M_CONST __FILE__ STRING(:__LINE__: MAGICCONSTANT UNDESIREABLE NO 1)

#include "Magic_global.h"

class MAGIC_EXPORT Magic
{
public:
    Magic();
};

#endif // MAGIC_H
