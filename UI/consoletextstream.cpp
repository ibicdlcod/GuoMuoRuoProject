#include "consoletextstream.h"

ConsoleTextStream::ConsoleTextStream()
    :QTextStream(stdout, QIODevice::WriteOnly)
{

}

ConsoleTextStream& ConsoleTextStream::operator<<(QString string)
{
#ifdef Q_OS_WIN
    // begin padding
    int width = QTextStream::fieldWidth();
    if(width > string.length())
    {
        int append_width = width - string.length();
        int left_append = append_width / 2;
        QTextStream::FieldAlignment alignment = this->fieldAlignment();
        switch (alignment) {
        case QTextStream::AlignLeft: string.append(QString(append_width, ' ')); break;
        case QTextStream::AlignAccountingStyle: // this isn't supported in this program, default.
        case QTextStream::AlignRight: string.prepend(QString(append_width, ' ')); break;
        case QTextStream::AlignCenter:
            string.prepend(QString(left_append, ' '));
            string.append(QString(append_width - left_append, ' ')); break;
        }
    }
    // end padding
    WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE),
                  string.utf16(), string.size(), NULL, NULL);
#else
    QTextStream::operator<<(string);
#endif
    QTextStream::flush();
    return *this;
}

inline ConsoleTextStream& ConsoleTextStream::operator<<(int input)
{
    QTextStream::operator<<(input);
    QTextStream::flush();
    return *this;
}

inline ConsoleTextStream & operator<<(ConsoleTextStream &s, QTextStreamManipulator &input)
{
    input.exec(s);
    return s;
}

void ConsoleTextStream::setFieldWidth(int width)
{
    QTextStream::setFieldWidth(width);
}

void ConsoleTextStream::setFieldAlignment(QTextStream::FieldAlignment alignment)
{
    QTextStream::setFieldAlignment(alignment);
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

