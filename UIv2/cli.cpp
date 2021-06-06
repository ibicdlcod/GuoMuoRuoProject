#include "cli.h"

#ifdef Q_OS_UNIX
#include <sys/ioctl.h> //ioctl() and TIOCGWINSZ
#include <unistd.h> // for STDOUT_FILENO
#endif

#include <QDir>
#include <QRegularExpression>
#include <iostream>
#include "ecma48.h"
#include "wcwidth.h"

/* Ugly as fuck, but customMessageHandler had to be a member of CLI in order to use tr() and then said function
 * must be static, but logFile isn't const at complie time, leaveing no other option
 */
extern QFile *logFile;

CLI::CLI(int argc, char ** argv)
    : QCoreApplication(argc, argv), timer(nullptr),
      qout(ConsoleTextStream(stdout, QIODevice::WriteOnly))
{

}

void CLI::customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QString dt = QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm:ss");
    QString txt = QStringLiteral("\r[%1] ").arg(dt);
    QByteArray localMsg = msg.toUtf8();
    const char *file = context.file ? context.file : "";
    const char *function = context.function ? context.function : "";

    QString txt2 = QStringLiteral("%1 (%2:%3, %4)").arg(tr(localMsg.constData()), file, QString::number(context.line), function);
    switch (type)
    {
    case QtDebugMsg:
        txt += QStringLiteral("{Debug} \t\t %1").arg(txt2);
        break;
    case QtInfoMsg:
        txt += QStringLiteral("{Info} \t\t %1").arg(txt2);
        break;
    case QtWarningMsg:
        txt += QStringLiteral("{Warning} \t %1").arg(txt2);
        break;
    case QtCriticalMsg:
        txt += QStringLiteral("{Critical} \t %1").arg(txt2);
        break;
    case QtFatalMsg:
        txt += QStringLiteral("{Fatal} \t\t %1").arg(txt2);
        abort();
        break;
    }

#ifdef Q_OS_WIN
    WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE),
                  txt.utf16(), txt.size(), NULL, NULL);
#else
    std::cout << txt.toUtf8().constData();
#endif
    std::cout << std::endl;

    if(!logFile->isWritable())
    {
        qFatal("Log file cannot be written to.");
    }

    if(txt.contains(QChar('\0')))
    {
        qFatal("Log Error");
    }
    QTextStream textStream(logFile);
    textStream << txt;
}

void CLI::openingwords()
{
    int width = getConsoleWidth();
    QString notice;
    QDir currentDir = QDir::current();
    QFile licenseFile(currentDir.filePath("openingwords.txt")); /* MAGICCONSTANT UNDESIREABLE NO 1 */
    if(!licenseFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qout << tr("Can't fild license file, exiting.") << Qt::endl;
        exitGracefully();
        return;
    }
    else
    {
        QTextStream instream1(&licenseFile);
        qout << Ecma48(255,255,255,true) << Ecma48(0,0,255);
        qout.setFieldAlignment(QTextStream::AlignCenter);
        while(!instream1.atEnd())
        {
            notice = instream1.readLine();
            qout.setFieldWidth(width);
            qout << notice;
            qout.setFieldWidth(0);
            qout << Qt::endl;
        }
        qout << EcmaSetter::AllDefault;
        qout.setFieldWidth(width);
        qout << "";
        qout.setFieldWidth(0);
        qout << Qt::endl;
        qout << Ecma48(192,255,192,true) << Ecma48(64,64,64);

        qout.setFieldWidth(width);

        /* This is considered an integral part of the program rather than magic constants. */
        qout << tr("What? Admiral Tanaka? He's the real deal, isn't he? Great at battle and bad at politics--so cool!");

        qout.setFieldWidth(0);
        qout << EcmaSetter::AllDefault << Qt::endl;
    }
    qout << EcmaSetter::AllDefault;
    qout.setFieldAlignment(QTextStream::AlignLeft);
    qout << Qt::endl;
}

bool CLI::parse(const QString &input)
{
    //qDebug() << input;
    QStringList commandParts = input.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    if(commandParts.length() > 0)
    {
        QString primary = commandParts[0];

        // aliases
        QMap<QString, QString> aliases;

        aliases["h"] = "help";
        aliases["q"] = "exit";
        aliases["c"] = "commands";

        if(aliases.contains(primary))
        {
            primary = aliases[primary];
        }
        // end aliases

        if(primary.compare("help", Qt::CaseInsensitive) == 0)
        {
            commandParts.removeFirst();
            showHelp(commandParts);
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
            bool success = parseSpec(commandParts);
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

void CLI::displayPrompt()
{
    qout << "ST$ ";
}

const QStringList CLI::getCommands()
{
    return {"exit", "help", "commands", "allcommands"};
}

void CLI::invalidCommand()
{
    qout << tr("Invalid Command, use 'commands' for valid commands, 'help' for help, 'exit' to exit.") << Qt::endl;
}

void CLI::showCommands(bool validOnly)
{

    qout << tr("Use 'exit' to quit.") << Qt::endl;
    if(validOnly)
    {
        qout << tr("Available commands:") << Qt::endl;
        qls(getValidCommands());
    }
    else
    {
        qout << tr("All commands:") << Qt::endl;
        qls(getCommandsSpec());
    }
}

void CLI::showHelp(const QStringList &)
{
    qout << tr("Use 'exit' to quit, 'help' to show help, 'commands' to show available commands.") << Qt::endl;
}

inline int CLI::callength(const QString &input, bool naive)
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

int CLI::getConsoleWidth()
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

/* Using Unicode characters in console AT ALL is recommended against
 * (https://stackoverflow.com/a/3971750)
 * TODO: Use QFontMetrics to handle
 * https://stackoverflow.com/questions/31732698/getting-font-metrics-without-gui-console-mode
 * TODO: Qt Console can't display unicode correctly
 * https://bugreports.qt.io/browse/QTCREATORBUG-8099
 */
void CLI::qls(const QStringList &input)
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
                qout.setFieldWidth(fieldwidth);
                QString str = *((displaySelected.begin() + j)->constBegin() + i);
                qout << str;
            }
        }
        qout.setFieldWidth(0);
        qout << Qt::endl;
    }
}

void CLI::exitGracefully()
{
    exitGraceSpec();
    if(timer)
    {
        timer->stop();
    }
    qout << Ecma48(64,255,64) << EcmaSetter::BlinkOn;
    qout << tr("Goodbye, press ENTER to quit") << EcmaSetter::AllDefault << Qt::endl;
    logFile->close();
    quit();
}
