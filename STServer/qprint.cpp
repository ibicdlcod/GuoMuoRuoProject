#include "qprint.h"

void qprint(QTextStream &stream, const char *in)
{
    while(*in != '\0')
    {
        stream << *in;
        in++;
    }
    stream << "" << Qt::endl;
}

void qprint(QTextStream &stream, QString in)
{
    stream << in << Qt::endl;
}
