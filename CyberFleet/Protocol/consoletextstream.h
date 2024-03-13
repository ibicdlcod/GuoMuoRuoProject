#ifndef CONSOLETEXTSTREAM_H
#define CONSOLETEXTSTREAM_H

#include <QIODevice>
#include <QRegularExpression>
#include <QSocketNotifier>
#include <QTextStream>
#include <type_traits>

#if defined (Q_OS_WIN)
#include <windows.h>
#include <iostream>
#endif
#if defined (Q_OS_UNIX)
#include <sys/ioctl.h> //ioctl() and TIOCGWINSZ
#include <unistd.h> // for STDOUT_FILENO
#endif

#include "ecma48.h"
#include "kp.h"
#include "wcwidth.h"

/* Usage of QTextStreamManipulator on this class is prohibited throughout
 * due to unfixable bugs, that is, qout << qSetFieldWidth result in
 * QTextStream & rather than this class.
 */
class ConsoleTextStream : public QTextStream
{
public:
    /* consider 'using QTextStream::QTextStream' */
    ConsoleTextStream(FILE *, QIODeviceBase::OpenMode);
    ConsoleTextStream& operator<<(QString);
    ConsoleTextStream& operator<<(int);
    ConsoleTextStream& operator<<(Ecma);
    inline void setFieldAlignment(QTextStream::FieldAlignment alignment) {
        QTextStream::setFieldAlignment(alignment);
    }

    /* Don't try this at home if you are a novice */
    /* https://stackoverflow.com/questions/48764158/ */
    /* https://stackoverflow.com/questions/495021/ */
    template <typename... Args>
    typename std::enable_if<(std::is_same<Args, Ecma>::value && ...), void>
    ::type printLine(const QString &input, Args&&... all)
    {
        /* Braces here isn't trivial: https://en.cppreference.com/w/cpp/language/fold */
#pragma message(USED_CXX17)
        (*this << ... << all);
        int width;
#if defined (Q_OS_WIN)
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
        width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
#elif defined (Q_OS_UNIX)
        struct winsize size;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
        width = size.ws_col;
#else
#pragma message(NOT_M_CONST)
        width = 80;
#endif
        /* Obviously, QString::SkipEmptyParts should not be used here, for a file may contain useful empty lines */
        static QRegularExpression e = QRegularExpression("[\r\n]");
        const QStringList noticeLines = input.split(e);
        for(QStringList::const_iterator i = noticeLines.constBegin();
            i != noticeLines.constEnd();
            ++i)
        {
            int length = i->length();
            wchar_t *data;
            data = reinterpret_cast<wchar_t *>(calloc(length, sizeof(wchar_t)));
            i->toWCharArray(data);
            int raw_length = mk_wcswidth(data, i->size());
            free(data);

            QTextStream::setFieldWidth(width * ((length - 1) / width + 1) - raw_length + length);
            *this << *i;
            QTextStream::setFieldWidth(0);
            *this << Qt::endl;
        }
        Ecma defa(EcmaSetter::AllDefault);
        *this << defa;
    }
    inline void printLine(const QString &input) {
        printLine(input, Ecma(EcmaSetter::AllDefault));
    }
    inline void print(const QString &input, int width) {
        QTextStream::setFieldWidth(width);
        *this << input;
    }
    void reset() {
        QTextStream::operator<<("\x1b[0m");
    }

};
#endif // CONSOLETEXTSTREAM_H
