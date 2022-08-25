#include "consoletextstream.h"

ConsoleTextStream::ConsoleTextStream(FILE *fileHandle, QIODeviceBase::OpenMode openMode = QIODevice::ReadWrite)
    :QTextStream(fileHandle, openMode) {

}

ConsoleTextStream& ConsoleTextStream::operator<<(QString string) {
#if defined (Q_OS_WIN)
    /* begin padding */
    int width = QTextStream::fieldWidth();
    if(width > string.length()) {
        int append_width = width - string.length();
        if(append_width < 0)
            append_width = 0;
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
    /* end padding */
    WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE),
                  string.utf16(), string.size(), NULL, NULL);
#else
    QTextStream::operator<<(string);
#endif
    QTextStream::flush();
    return *this;
}

ConsoleTextStream& ConsoleTextStream::operator<<(int input) {
    QTextStream::operator<<(input);
    QTextStream::flush();
    return *this;
}

ConsoleTextStream& ConsoleTextStream::operator<<(Ecma input) {
    QString code = QStringLiteral("");
    switch(input.member) {
    case structInt::ecmaSetter:
        code.append("\x1b[");
        switch(input.mem1.setter) {
        case EcmaSetter::UnderscoreOn:  code.append("4");   break;
        case EcmaSetter::UnderscoreOff: code.append("24");  break;
        case EcmaSetter::BlinkOn:       code.append("5");   break;
        case EcmaSetter::BlinkOff:      code.append("25");  break;
        case EcmaSetter::ReverseVideo:  code.append("7");   break;
        case EcmaSetter::NormalVideo:   code.append("27");  break;
        case EcmaSetter::TextDefault:   code.append("39");  break;
        case EcmaSetter::BgDefault:     code.append("49");  break;
        case EcmaSetter::AllDefault:    code.append("0");   break;
        case EcmaSetter::ItalicsOn:     code.append("3");   break;
        case EcmaSetter::ItalicsOff:    code.append("23");  break;
        case EcmaSetter::DoubleScore:   code.append("21");  break;
        case EcmaSetter::Invisible:     code.append("8");   break;
        case EcmaSetter::Visible:       code.append("28");  break;
        case EcmaSetter::CrossedOut:    code.append("9");   break;
        case EcmaSetter::NotCrossedOut: code.append("29");  break;
        case EcmaSetter::OverlineOn:    code.append("53");  break;
        case EcmaSetter::OverlineOff:   code.append("55");  break;
        }
        code.append("m");
        break;
    case structInt::ecma48:
        code.append("\x1b[");
        code.append(input.mem2.color.background ? "48" : "38");
        code.append(";2;");
        code.append(QString::number(input.mem2.color.red));
        code.append(";");
        code.append(QString::number(input.mem2.color.green));
        code.append(";");
        code.append(QString::number(input.mem2.color.blue));
        code.append("m");
        break;
    case structInt::qtextStreamManipulator:
        input.mem3.manipulator.exec(*this);
        return *this;
        Q_UNREACHABLE();
        break;
    }
    int currentwidth = this->fieldWidth();
    QTextStream::setFieldWidth(0);
    QTextStream::operator<<(code);
    QTextStream::setFieldWidth(currentwidth);
    this->flush();
    return *this;
}
