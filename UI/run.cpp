#include <QFile>
#include <QDir>

#include "run.h"
#include "qprint.h"
#include "../logic/strelations.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif
#ifdef Q_OS_UNIX
#include <sys/ioctl.h> //ioctl() and TIOCGWINSZ
#include <unistd.h> // for STDOUT_FILENO
#endif

#include <iostream>

Run::Run(QObject *parent)
    : QObject(parent), qout(ConsoleTextStream()), qin(ConsoleInput())
{

}

void Run::run()
{

}

void Run::invalidCommand()
{
    qPrint(qout, "Invalid Command, use 'showValid' for valid commands, 'help' for help, 'exit' to exit.");
}

void Run::showAllCommands()
{
    qPrint(qout, "Use 'exit' to quit.");
    qPrint(qout, "");
    qPrint(qout, "All commands:");
    QStringList all;
    for(STCType c : STCType::_values())
    {
        all.append(c._to_string());
    }
    qls(all);
    qPrint(qout, "");
    qPrint(qout, "Use the following to change state:");
    QStringList states;
    for(STState s : STState::_values())
    {
        states.append(s._to_string());
    }
    qls(states);
    qPrint(qout, "");
}

void Run::showCommands(const QList<STCType> valids)
{
    qPrint(qout, "Use 'exit' to quit.");
    qPrint(qout, "");
    qPrint(qout, "Available commands:");
    QStringList validList;
    QList<STCType>::const_iterator i;
    for(i = valids.constBegin(); i != valids.constEnd(); ++i)
    {
        validList.append(i->_to_string());
    }
    qls(validList);
    qPrint(qout, "");
    qPrint(qout, "Use the following to change state:");
    QStringList states;
    for(STState s : STState::_values())
    {
        states.append(s._to_string());
    }
    qls(states);
    qPrint(qout, "");
}

void Run::showHelp(QStringList parameters)
{
    if(parameters.length() == 0)
    {
        qPrint(qout, "Generic help message");
    }
    else
    {
        qPrint(qout, "Specific help message");
    }
    qls(QStringList());
    qPrint(qout, "");
}

/* The following function will NOT work for East Asian characters.
 * however, using Unicode characters in console AT ALL is recommended against
 * (https://stackoverflow.com/a/3971750)
 * TBD: Use QFontMetrics to handle
 * https://stackoverflow.com/questions/31732698/getting-font-metrics-without-gui-console-mode
 * TBD: Qt Console can't display unicode correctly
 * https://bugreports.qt.io/browse/QTCREATORBUG-8099
 */
int Run::getConsoleWidth()
{
    int width;
#ifdef Q_OS_WIN
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
#else
#ifdef Q_OS_UNIX
    struct winsize size;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
    width = size.ws_col;
#else
    width = 80;
#endif
#endif
    return width;
}

template<class T>
void Run::qls(const QList<T> input)
{
    if(input.length() <= 0)
    {
        return;
    }
    int width;
#ifdef Q_OS_WIN
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
#else
#ifdef Q_OS_UNIX
    struct winsize size;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
    width = size.ws_col;
#else
    width = 80;
#endif
#endif

    auto lengthcmp = [](T a, auto&& b) { return callength(a) < callength(b); };

    int maxcolumns = input.length();
    int total = input.length();
    QMap<int, QList<QList<T>>> displayCandidates;
    for(int columns = 1; columns <= maxcolumns; ++columns)
    {
        int rows;
        if(total % columns == 0)
        {
            rows = total / columns;
        }
        else
        {
            rows = total / columns + 1;
        }
        QList<QList<T>> displays;
        QList<T> singlecolumn;
        typedef typename QList<T>::const_iterator iter;
        for(iter i = input.constBegin(); i != input.constEnd(); ++i)
        {
            if(singlecolumn.length() < rows && i != (input.constEnd() - 1))
            {
                singlecolumn.append(*i);
            }
            else if (singlecolumn.length() < rows && i == (input.constEnd() - 1))
            {
                singlecolumn.append(*i);
                displays.append(singlecolumn);
            }
            else
            {
                displays.append(singlecolumn);
                singlecolumn = *(new QList<T>());
                --i;

            }
        }
        int displayedcolumns = 0;
        for(int j = 0; j < displays.length(); ++j)
        {
            displayedcolumns += (callength(*std::max_element(
                                               (displays.constBegin() + j)->constBegin(),
                                               (displays.constBegin() + j)->constEnd(),
                                               lengthcmp)) + 1);
        }
        displayedcolumns--;

        if(displayedcolumns <= width)
        {
            displayCandidates[rows] = displays;

        }
    }
    QList<int> k = displayCandidates.keys();
    int min = *std::min_element(k.begin(), k.end());
    QList<QList<T>> displaySelected = displayCandidates[min];

    for(int i = 0; i < displaySelected.begin()->length(); ++i)
    {
        for(int j = 0; j < displaySelected.length(); ++j)
        {
            if(i < (displaySelected.begin() + j)->length())
            {
                T current_element = *((displaySelected.begin() + j)->constBegin() + i);
                int fieldwidth = callength(*std::max_element(
                                               (displaySelected.begin() + j)->begin(),
                                               (displaySelected.begin() + j)->end(),
                                               lengthcmp)) + ((j == displaySelected.length() - 1) ? 0 : 1)
                        - callength(current_element)
                        + callength(current_element, true);
                qout.setFieldWidth(fieldwidth);
                QString str = strfiy(*((displaySelected.begin() + j)->constBegin() + i));
                qout << str;
            }
        }
        qout.setFieldWidth(0);
        qout << Qt::endl;
    }
    qout << Qt::endl;
}

inline int Run::callength(const QString &input, bool naive)
{
    if(naive)
    {
        return input.length();
    }
    else
    {
        const QChar *data = input.constData();
        return mk_wcswidth(data, input.size());
    }
}

inline int Run::callength(const QHostAddress &input, bool naive)
{
    Q_UNUSED(naive)
    return input.toString().length();
}

inline const QString & Run::strfiy(const QString &in)
{
    return in;
}

inline QString Run::strfiy(const QHostAddress &in)
{
    QString result = in.toString();
    return result;
}
