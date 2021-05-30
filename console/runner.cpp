#include <QFile>
#include <QDir>

#include "runner.h"
#include "qprint.h"
#include "logic/stengine.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif
#ifdef Q_OS_UNIX
#include <sys/ioctl.h> //ioctl() and TIOCGWINSZ
#include <unistd.h> // for STDOUT_FILENO
#endif

#include <iostream>

void Runner::run()
{
    QString notice;
    QFile openingwords(QDir::current().filePath("openingwords.txt"));
    if(!openingwords.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qprint("Opening words file not found, exiting.");
        emit finished();
        return;
    }
    else
    {
        QTextStream instream1(&openingwords);
        notice = instream1.readAll();
    }

    QString cmdline;

    qprint(notice);

    STEngine engine;
    QObject::connect(&engine, SIGNAL(invalidCommand()), this, SLOT(invalidCommand()));
    QObject::connect(&engine, SIGNAL(showAllCommands()), this, SLOT(showAllCommands()));
    QObject::connect(&engine, SIGNAL(showCommands(QList<STCType>)), this, SLOT(showCommands(QList<STCType>)));
    QObject::connect(&engine, SIGNAL(showHelp(QStringList)), this, SLOT(showHelp(QStringList)));
    while(true)
    {
        QTextStream(stdout) << "ST " + engine.getState() + "$ ";
        cmdline = QTextStream(stdin).readLine();
        if(cmdline.compare("exit") == 0)
        {
            qprint("ST ended, press ENTER to quit");
            emit finished();
            return;
        }
        engine.parse(cmdline);

        //TBD: record in history

    }
}

void Runner::invalidCommand()
{
    qprint("Invalid Command, use 'showValid' for valid commands, 'help' for help, 'exit' to exit.");
}

void Runner::showAllCommands()
{
    qprint("Use 'exit' to quit.");
    qprint("");
    qprint("All commands:");
    QStringList all;
    for(STCType c : STCType::_values())
    {
        all.append(c._to_string());
    }
    qls(all);
    qprint("");
    qprint("Use the following to change state:");
    QStringList states;
    for(STState s : STState::_values())
    {
        states.append(s._to_string());
    }
    qls(states);
    qprint("");
}

void Runner::showCommands(const QList<STCType> valids)
{
    qprint("Use 'exit' to quit.");
    qprint("");
    qprint("Available commands:");
    QStringList validList;
    QList<STCType>::const_iterator i;
    for(i = valids.constBegin(); i != valids.constEnd(); ++i)
    {
        validList.append(i->_to_string());
    }
    qls(validList);
    qprint("");
    qprint("Use the following to change state:");
    QStringList states;
    for(STState s : STState::_values())
    {
        states.append(s._to_string());
    }
    qls(states);
    qprint("");
}

void Runner::showHelp(QStringList parameters)
{
    if(parameters.length() == 0)
    {
        qprint("Generic help message");
    }
    else
    {
        qprint("Specific help message");
    }
    qls(QStringList());
    qprint("");
}

/* The following function will NOT work for East Asian characters.
 * however, using Unicode characters in console AT ALL is recommended against
 * (https://stackoverflow.com/a/3971750)
 * TBD: Use QFontMetrics to handle
 * https://stackoverflow.com/questions/31732698/getting-font-metrics-without-gui-console-mode
 * TBD: Qt Console can't display unicode correctly
 * https://bugreports.qt.io/browse/QTCREATORBUG-8099
 */

void Runner::qls(const QStringList input)
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

    int maxcolumns = input.length();
    int total = input.length();
    QMap<int, QList<QStringList>> displayCandidates;
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
        QList<QStringList> displays;
        QStringList singlecolumn;
        for(QStringList::const_iterator i = input.constBegin(); i != input.constEnd(); ++i)
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
                singlecolumn = *(new QStringList());
                --i;

            }
        }
        int displayedcolumns = 0;
        for(int j = 0; j < displays.length(); ++j)
        {
            displayedcolumns += std::max_element(
                        (displays.constBegin() + j)->constBegin(),
                        (displays.constBegin() + j)->constEnd(),
                        lengthcmp)->length() + 1;
        }
        displayedcolumns--;

        if(displayedcolumns <= width)
        {
            displayCandidates[rows] = displays;

        }
    }
    QList<int> k = displayCandidates.keys();
    int min = *std::min_element(k.begin(), k.end());
    QList<QStringList> displaySelected = displayCandidates[min];
    QTextStream qout = QTextStream(stdout);
    qout.setFieldAlignment(QTextStream::AlignLeft);
    for(int i = 0; i < displaySelected.begin()->length(); ++i)
    {
        for(int j = 0; j < displaySelected.length(); ++j)
        {
            if(i < (displaySelected.begin() + j)->length())
            {
                qout << qSetFieldWidth(std::max_element(
                                           (displaySelected.begin() + j)->begin(),
                                           (displaySelected.begin() + j)->end(),
                                           lengthcmp)->length() + ((j == displaySelected.length() - 1) ? 0 : 1))
                     << *((displaySelected.begin() + j)->constBegin() + i);
            }
        }
        qout << qSetFieldWidth(0) << Qt::endl;
    }
}

bool Runner::lengthcmp(const QString &left, const QString &right)
{
    return left.length() < right.length();
}
