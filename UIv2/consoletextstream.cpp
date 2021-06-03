#include "consoletextstream.h"

ConsoleTextStream::ConsoleTextStream(FILE *fileHandle, QIODeviceBase::OpenMode openMode = QIODevice::ReadWrite)
    :QTextStream(fileHandle, openMode)
    //:QTextStream(stdout, QIODevice::WriteOnly)
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
