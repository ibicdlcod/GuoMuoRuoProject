#ifndef CONSOLETEXTSTREAM_H
#define CONSOLETEXTSTREAM_H

#include <QTextStream>

#if defined (Q_OS_WIN)
#include <windows.h>
#include <iostream>
#endif

#include <QIODevice>
#include <QSocketNotifier>

/* Usage of QTextStreamManipulator on this class is prohibited throughout due to unfixable bugs,
 * that is, qout << qSetFieldWidth result in QTextStream & rather than this class.
 */
class ConsoleTextStream : public QTextStream
{
public:
    ConsoleTextStream(FILE *, QIODeviceBase::OpenMode);
    ConsoleTextStream& operator<<(QString);
    ConsoleTextStream& operator<<(int);
    void setFieldWidth(int width);
    void setFieldAlignment(QTextStream::FieldAlignment alignment);
};

/* As stated above, isn't working as intended */
ConsoleTextStream & operator<<(ConsoleTextStream &s, QTextStreamManipulator &input);

#endif // CONSOLETEXTSTREAM_H
