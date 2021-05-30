#ifndef QPRINT_H
#define QPRINT_H

#include <QTextStream>

void qprint(QTextStream &stream, const char *in);

void qprint(QTextStream &stream, QString in);

#endif // QPRINT_H
