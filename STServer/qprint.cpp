#include "qprint.h"

void qPrint(ConsoleTextStream &stream, const char *in)
{
    stream << QString::fromUtf8(in) << Qt::endl;
}

void qPrint(ConsoleTextStream &stream, QString in)
{
    stream << in << Qt::endl;
}
