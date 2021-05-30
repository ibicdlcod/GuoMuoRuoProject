#include "consoletextstream.h"

ConsoleTextStream::ConsoleTextStream()
    :QTextStream(stdout, QIODevice::WriteOnly)
{

}

ConsoleTextStream& ConsoleTextStream::operator<<(const QString &string)
{
#ifdef Q_OS_WIN
    WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE),
                  string.utf16(), string.size(), NULL, NULL);
#else
    QTextStream::operator<<(string);
#endif
    return *this;
}

ConsoleInput::ConsoleInput()
    :QTextStream(stdin, QIODevice::ReadOnly)
{
}

QString ConsoleInput::readline()
{
#ifdef Q_OS_WIN32
    const int bufsize = 512;
    wchar_t buf[bufsize];
    DWORD read;
    QString res;
    do {
        ReadConsoleW(GetStdHandle(STD_INPUT_HANDLE),
                     buf, bufsize, &read, NULL);
        res += QString::fromWCharArray(buf, read);
    } while (read > 0 && res[res.length() - 1] != '\n');
    // could just do res.truncate(res.length() - 2), but better be safe
    while (res.length() > 0
           && (res[res.length() - 1] == '\r' || res[res.length() - 1] == '\n'))
        res.truncate(res.length() - 1);
    return res;
#else
    return QTextStream::readLine();
#endif
}
