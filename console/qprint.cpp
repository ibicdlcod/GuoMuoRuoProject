#include "qprint.h"

void qprint(const char *in)
{
    while(*in != '\0')
    {
        QTextStream(stdout) << *in;
        in++;
    }
    QTextStream(stdout) << "" << Qt::endl;
}

void qprint(QString in)
{
    QTextStream(stdout) << in << Qt::endl;
}
