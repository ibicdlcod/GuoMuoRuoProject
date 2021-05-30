#ifndef CONSOLETEXTSTREAM_H
#define CONSOLETEXTSTREAM_H

#include <QTextStream>
#include <QIODevice>

#ifdef Q_OS_WIN
#include <windows.h>
#include <iostream>
#endif

class ConsoleTextStream : public QTextStream
{
public:
    ConsoleTextStream();
    ConsoleTextStream& operator<<(const QString &);
};

class ConsoleInput : public QTextStream
{
public:
    ConsoleInput();
    QString readline();
};

#endif // CONSOLETEXTSTREAM_H
