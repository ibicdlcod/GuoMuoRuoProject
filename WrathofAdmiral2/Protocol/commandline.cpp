#include "commandline.h"

#if defined (Q_OS_WIN)
#include <windows.h>
#endif
#if defined (Q_OS_UNIX)
#include <sys/ioctl.h> //ioctl() and TIOCGWINSZ
#include <unistd.h> // for STDOUT_FILENO
#endif

#include <QDir>
#include <QRegularExpression>
#include <QSettings>
#include <iostream>

#include "consoletextstream.h"
#include "wcwidth.h"
#include "kp.h"

/* Ugly as fuck, but customMessageHandler had to be a member of CommandLine in order to use tr() and then said function
 * must be static, but logFile isn't const at complie time, leaveing no other option
 *
 * (Outdated, se QT_TR_NOOP() and QT_TRANSLATE_NOOP())
 */
extern QFile *logFile;
extern QSettings *settings;

CommandLine::CommandLine(int argc, char ** argv)
    : QCoreApplication(argc, argv), timer(nullptr),
      qout(ConsoleTextStream(stdout, QIODevice::WriteOnly)),
      passwordMode(Password::normal)
{
}

void CommandLine::customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg_original)
{
    QString msg = msg_original;
    if(msg.endsWith("\n"))
    {
        msg.remove(msg.length() - 1, 1);
    }
    msg.remove(QChar('\"'), Qt::CaseInsensitive);
    QString dt = QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm:ss");
    QString txt = QStringLiteral("\r");
    QByteArray localMsg = msg.toUtf8();
    const char *file = context.file ? context.file : "";
    const char *function = context.function ? context.function : "";
    bool msg_off = false;

#if defined(QT_DEBUG)
    QString txt2 = QStringLiteral("%1 (%2:%3, %4)").arg(localMsg, file, QString::number(context.line), function);
#else
    Q_UNUSED(file)
    Q_UNUSED(function)
    QString txt2 = QStringLiteral("%1").arg(localMsg);
#endif
    switch (type)
    {
    case QtDebugMsg:
        txt += QStringLiteral("{DEBUG} %1").arg(txt2);
        msg_off = settings->value("msg_disabled/debug", false).toBool();
        break;
    case QtInfoMsg:
        txt += QStringLiteral("{INFO}  %1").arg(txt2);
        msg_off = settings->value("msg_disabled/info", false).toBool();
        break;
    case QtWarningMsg:
        txt += QStringLiteral("{WARN}  %1").arg(txt2);
        msg_off = settings->value("msg_disabled/warn", false).toBool();
        break;
    case QtCriticalMsg:
        txt += QStringLiteral("{CRIT}  %1").arg(txt2);
        msg_off = settings->value("msg_disabled/crit", false).toBool();
        break;
    case QtFatalMsg:
        txt += QStringLiteral("{FATAL} %1").arg(txt2);
        msg_off = settings->value("msg_disabled/fatal", false).toBool();
        break;
    }
    /* consider use QT_NO_DEBUG_OUTPUT, QT_NO_INFO_OUTPUT, QT_NO_WARNING_OUTPUT */

    const char * color;
    switch(type)
    {
    case QtDebugMsg:
        color = ("\x1b[48;2;0;255;0;30m");
        break;
    case QtInfoMsg:
        color = ("\x1b[48;2;0;0;255;97m");
        break;
    case QtWarningMsg:
        color = ("\x1b[48;2;255;255;0;30m");
        break;
    case QtCriticalMsg:
        color = ("\x1b[48;2;255;0;0;97m");
        break;
    case QtFatalMsg:
        color = ("\x1b[48;2;128;0;0;97m");
        break;
    }

    if(!msg_off)
    {
        std::cout << color;

#if defined (Q_OS_WIN)
        WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE),
                      txt.utf16(), txt.size(), NULL, NULL);
#else
        std::cout << txt.toUtf8().constData();
#endif
        std::cout << "\x1b[49;39m" << std::endl;
    }

    if(!logFile->isWritable())
    {
        qFatal("Log file cannot be written to.");
    }

    if(txt.contains(QChar('\0')))
    {
        qFatal("Log Error");
    }
    QTextStream textStream(logFile);
    txt.remove(QChar('\r'), Qt::CaseInsensitive);
    txt = QStringLiteral("[%1] %2\n").arg(dt, txt);
    textStream << txt;
    if(type == QtFatalMsg)
    {
        abort();
    }
}

void CommandLine::openingwords()
{
    QString notice;
    QDir currentDir = QDir::current();
    /* the default is qt resource system */
    QString openingwords = settings->value("license_notice", ":/openingwords.txt").toString();
    QFile licenseFile(currentDir.filePath(openingwords));
    if(!licenseFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        /* TODO: check MD5 */
        qout << qtTrId("licence-not-found") << Qt::endl;
        qout.setFieldAlignment(QTextStream::AlignLeft);
        exitGracefully();
        return;
    }
    else
    {
        QTextStream instream1(&licenseFile);

        qout.setFieldAlignment(QTextStream::AlignCenter);

        notice = instream1.readAll();
        qout.printLine(notice, Ecma(255,255,255,true), Ecma(0,0,255));
        qout.printLine("");
#pragma message(NOT_M_CONST)
        //% "What? Admiral Tanaka? He's the real deal, isn't he?\nGreat at battle and bad at politics--so cool!"
        qout.printLine(qtTrId("naganami"), Ecma(192,255,192,true), Ecma(64,64,64));
    }
    qout.setFieldAlignment(QTextStream::AlignLeft);
}

bool CommandLine::parse(const QString &input)
{
    if(passwordMode)
    {
        bool success = parseSpec(QStringList(input));
        if(!success)
        {
            invalidCommand();
            displayPrompt();
            return false;
        }
        else
        {
            displayPrompt();
            return true;
        }
    }
    static QRegularExpression re("\\s+");
    QStringList cmdParts = input.split(re, Qt::SkipEmptyParts);
    if(cmdParts.length() > 0)
    {
        QString primary = cmdParts[0];

        /* aliases */
        QMap<QString, QString> aliases;
        aliases["h"] = "help";
        aliases["q"] = "exit";
        aliases["c"] = "commands";
        aliases["a"] = "allcommands";

        if(aliases.contains(primary))
        {
            primary = aliases[primary];
        }
        /* end aliases */

        if(primary.compare("help", Qt::CaseInsensitive) == 0)
        {
            cmdParts.removeFirst();
            showHelp(cmdParts);
            displayPrompt();
            return true;
        }
        else if(primary.compare("exit", Qt::CaseInsensitive) == 0)
        {
            exitGracefully();
            return true;
        }
        else if(primary.compare("commands", Qt::CaseInsensitive) == 0)
        {
            showCommands(true);
            displayPrompt();
            return true;
        }
        else if(primary.compare("allcommands", Qt::CaseInsensitive) == 0)
        {
            showCommands(false);
            displayPrompt();
            return true;
        }
        else
        {
            bool success = parseSpec(cmdParts);
            if(!success)
            {
                invalidCommand();
                displayPrompt();
                return false;
            }
            else
            {
                displayPrompt();
                return true;
            }
        }
    }
    displayPrompt();
    return false;
}

const QStringList CommandLine::getCommands()
{
    return {"exit", "help", "commands", "allcommands"};
}

inline void CommandLine::invalidCommand()
{
    //% "Invalid Command, use 'commands' for valid commands, 'help' for help, 'exit' to exit."
    qout << qtTrId("invalid-command") << Qt::endl;
}

void CommandLine::showCommands(bool validOnly)
{
    //% "Use 'exit' to quit."
    qout << qtTrId("exit-helper") << Qt::endl;
    if(validOnly)
    {
        //% "Available commands:"
        qout.printLine(qtTrId("good-command"), Ecma(0,255,0));
        qls(getValidCommands());
    }
    else
    {
        //% "All commands:"
        qout.printLine(qtTrId("all-command"), Ecma(255,255,0));
        qls(getCommandsSpec());
    }
}

inline void CommandLine::showHelp(const QStringList &)
{
    //% "Use 'exit' to quit, 'help' to show help, 'commands' to show available commands."
    qout << qtTrId("help-msg") << Qt::endl;
}

inline int CommandLine::callength(const QString &input, bool naive)
{
    if(naive)
    {
        return input.length();
    }
    else
    {
        int length = input.length();
        wchar_t *data;
        data = reinterpret_cast<wchar_t *>(calloc(length, sizeof(wchar_t)));
        input.toWCharArray(data);
        return mk_wcswidth(data, length);
    }
}

int CommandLine::getConsoleWidth()
{
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
    width = 80;
#endif
    return width;
}

/* Using Unicode characters in console AT ALL is recommended against
 * (https://stackoverflow.com/a/3971750)
 * TODO: Use QFontMetrics to handle
 * https://stackoverflow.com/questions/31732698/getting-font-metrics-without-gui-console-mode
 * TODO: Qt Console can't display unicode correctly
 * https://bugreports.qt.io/browse/QTCREATORBUG-8099
 */
void CommandLine::qls(const QStringList &input)
{
    if(input.length() <= 0)
    {
        return;
    }
    int width = getConsoleWidth();

    auto lengthcmp = [](QString a, auto&& b) { return callength(a) < callength(b); };

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
        typedef typename QStringList::const_iterator iter;
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
                singlecolumn = *(new QStringList());
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
    QList<QStringList> displaySelected = displayCandidates[min];

    for(int i = 0; i < displaySelected.begin()->length(); ++i)
    {
        for(int j = 0; j < displaySelected.length(); ++j)
        {
            if(i < (displaySelected.begin() + j)->length())
            {
                QString current_element = *((displaySelected.begin() + j)->constBegin() + i);
                int fieldwidth = callength(*std::max_element(
                                               (displaySelected.begin() + j)->begin(),
                                               (displaySelected.begin() + j)->end(),
                                               lengthcmp)) + ((j == displaySelected.length() - 1) ? 0 : 1)
                        - callength(current_element)
                        + callength(current_element, true);
                QString str = *((displaySelected.begin() + j)->constBegin() + i);
                qout.print(str, fieldwidth);
            }
        }
        qout.print("", 0);
        qout << Qt::endl;
    }
}

void CommandLine::exitGracefully()
{
    exitGraceSpec();
    if(timer)
    {
        timer->stop();
    }
    //% "Goodbye, press ENTER to quit."
    qout.printLine(qtTrId("goodbye"), Ecma(64,255,64), Ecma(EcmaSetter::BlinkOn));
    qout.reset();
    logFile->close();
    quit();
}
