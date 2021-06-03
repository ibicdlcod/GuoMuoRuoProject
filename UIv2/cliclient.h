#ifndef CLICLIENT_H
#define CLICLIENT_H

#include "cli.h"

class CliClient : public CLI
{
public:
    explicit CliClient(int, char **);
};

#endif // CLICLIENT_H
