#include "ecma48.h"

Ecma48::Ecma48(quint8 red,
               quint8 green,
               quint8 blue,
               bool background)
    : red(red), green(green), blue(blue), background(background)
{

}

ConsoleTextStream & operator<<(ConsoleTextStream &stream, EcmaSetter in)
{
    QString code;
    switch (in) {
    case EcmaSetter::UnderscoreOn:  code = "4";     break;
    case EcmaSetter::UnderscoreOff: code = "24";    break;
    case EcmaSetter::BlinkOn:       code = "5";     break;
    case EcmaSetter::BlinkOff:      code = "25";    break;
    case EcmaSetter::ReverseVideo:  code = "7";     break;
    case EcmaSetter::NormalVideo:   code = "27";    break;
    case EcmaSetter::TextDefault:   code = "39";    break;
    case EcmaSetter::BgDefault:     code = "49";    break;
    case EcmaSetter::AllDefault:    code = "0";     break;
    case EcmaSetter::ItalicsOn:     code = "3";     break;
    case EcmaSetter::ItalicsOff:    code = "23";    break;
    case EcmaSetter::DoubleScore:   code = "21";    break;
    case EcmaSetter::Invisible:     code = "8";     break;
    case EcmaSetter::Visible:       code = "28";    break;
    case EcmaSetter::CrossedOut:    code = "9";     break;
    case EcmaSetter::NotCrossedOut: code = "29";    break;
    case EcmaSetter::OverlineOn:    code = "53";    break;
    case EcmaSetter::OverlineOff:   code = "55";    break;
    }
    int currentwidth = stream.fieldWidth();
    stream.setFieldWidth(0);
    stream << "\x1b[" + code + "m";
    stream.setFieldWidth(currentwidth);
    stream.flush();
    return stream;
}

ConsoleTextStream & operator<<(ConsoleTextStream &stream, Ecma48 in)
{
    QString code;
    code.append("\x1b[");
    code.append(in.background ? "48" : "38");
    code.append(";2;");
    code.append(QString::number(in.red));
    code.append(";");
    code.append(QString::number(in.green));
    code.append(";");
    code.append(QString::number(in.blue));
    code.append("m");
    int currentwidth = stream.fieldWidth();
    stream.setFieldWidth(0);
    stream << code;
    stream.setFieldWidth(currentwidth);
    stream.flush();
    return stream;
}
