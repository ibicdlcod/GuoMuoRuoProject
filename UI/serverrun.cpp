#include <QFile>
#include <QDir>
#include <QRegularExpression>

#ifdef Q_OS_WIN
#include <windows.h>
#endif
#ifdef Q_OS_UNIX
#include <sys/ioctl.h> //ioctl() and TIOCGWINSZ
#include <unistd.h> // for STDOUT_FILENO
#endif

#include <QCoreApplication>

#include "serverrun.h"
#include "qprint.h"
#include "ecma48.h"

ServerRun::ServerRun(QObject *parent)
    : QObject(parent), qout(ConsoleTextStream()), qin(ConsoleInput()), server(nullptr)
{
}

void ServerRun::run()
{
    int width = getConsoleWidth();
    QString notice;
    QDir serverDir = QDir::current();
    QFile openingwords(serverDir.filePath("openingwords.txt"));
    if(!openingwords.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qPrint(qout, tr("Opening words file not found, exiting."));
        emit finished();
        return;
    }
    else
    {
        QTextStream instream1(&openingwords);
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
        qout << Qt::endl;
        qout << Ecma48(192,255,192,true) << Ecma48(64,64,64);

        qout.setFieldWidth(width);
        qout << tr("What? Admiral Tanaka? He's the real deal, isn't he? Great at battle and bad at politics--so cool!");
        qout.setFieldWidth(0);
        qout << Qt::endl;
    }
    qout << EcmaSetter::AllDefault;
    qout.setFieldAlignment(QTextStream::AlignLeft);
    qout << Qt::endl;

    timer = new QTimer(this);
    QObject::connect(timer, &QTimer::timeout, this, &ServerRun::update);
    timer->start(1000);

    QString cmdline;
    while(true)
    {
        update();
        qout << "STS ";
        qout << ((server && server->isWritable()) ? "RUNNING" : "NOTRUNNING");
        qout << "$ ";
        cmdline = qin.readline();

        if(cmdline.compare("start") == 0)
        {
            server = new QProcess();
            bool success = QObject::connect(server, &QProcess::errorOccurred,
                                            this, &ServerRun::processError)
                    && QObject::connect(server, &QProcess::started,
                                        this, &ServerRun::serverStarted)
                    && QObject::connect(server, &QProcess::stateChanged,
                                        this, &ServerRun::serverChanged)
                    && QObject::connect(server, &QProcess::finished,
                                        this, &ServerRun::processFinished)
                    && QObject::connect(server, &QProcess::readyReadStandardError,
                                        this, &ServerRun::serverStderr)
                    && QObject::connect(server, &QProcess::readyReadStandardOutput,
                                        this, &ServerRun::serverStderr);
            if(!success)
            {
                qFatal("Communication with server process can't be established.");
            }
            server->start("build-Server-Desktop_Qt_6_1_0_MinGW_64_bit-Debug/debug/Server.exe",
                          {"192.168.0.3", QString::number(1826)}, QIODevice::ReadWrite);
        }
        if(cmdline.compare("exit") == 0)
        {
            exitGracefully();
            return;
        }
        //parse(cmdline);

        //TBD: record in history
    }
}

void ServerRun::update()
{
    QCoreApplication::processEvents();
    qout.flush();
}

void ServerRun::processError(QProcess::ProcessError error)
{
    switch(error)
    {
    case QProcess::FailedToStart:
        qCritical("Server failed to start."); break;
    case QProcess::Crashed:
        qCritical("Server crashed."); break;
    case QProcess::Timedout:
        qCritical("Server function timed out."); break;
    case QProcess::WriteError:
        qCritical("Error writing to the server process."); break;
    case QProcess::ReadError:
        qCritical("Error reading from the server process."); break;
    case QProcess::UnknownError:
        qCritical("Server had unknown error."); break;
    }
}

void ServerRun::processFinished(int exitcode, QProcess::ExitStatus exitst)
{
    switch(exitst)
    {
    case QProcess::NormalExit:
        qInfo("Server finished normally with exit code %d", exitcode);
        break;
    case QProcess::CrashExit:
        qCritical("Server crashed."); break;
    }
}

void ServerRun::serverStderr()
{
    QByteArray output = server->readAllStandardError();
    const char * output_str = output.constData();
    switch(output_str[7])
    {
    case 'E': qCritical("%s", output.constData()); break;
    case 'W': qWarning("%s", output.constData()); break;
    case 'I': qInfo("%s", output.constData()); break;
    default: qCritical("%s", output.constData()); break;
    }
    qPrint(qout, QString(output));
}

void ServerRun::serverStdout()
{
    QByteArray output = server->readAllStandardOutput();
    qInfo("%s", output.constData());
    qPrint(qout, QString(output));
}

void ServerRun::serverStarted()
{
    qPrint(qout, "Server started and running.");
}

void ServerRun::serverChanged(QProcess::ProcessState newstate)
{
    switch(newstate)
    {
    case QProcess::NotRunning:
        qCritical("Server->NotRunning."); break;
    case QProcess::Starting:
        qInfo("Server->Starting."); break;
    case QProcess::Running:
        qInfo("Server->Running."); break;
    }
}

void ServerRun::shutdownServer()
{
    int waitformsec = 12000;
    if(server && server->isWritable())
    {
        server->write("SIGTERM\n");
        qPrint(qout, tr("Waiting for server finish..."));
        if(!server->waitForFinished(waitformsec))
        {
            qPrint(qout, tr("Server isn't responding after %d msecs, killing.").arg(waitformsec));
            server->kill();
        }
    }
    update();
}

void ServerRun::exitGracefully()
{
    timer->stop();
    shutdownServer();
    //update();
    qout << EcmaSetter::AllDefault;
    qPrint(qout, tr("STS ended, press ENTER to quit"));
    emit finished();
}

bool ServerRun::parse(QString command)
{
    QStringList commandParts = command.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    if(commandParts.length() > 0)
    {
        QString primary = commandParts[0];

        // aliases
        QMap<QString, QString> aliases;

        if(aliases.contains(primary))
        {
            primary = aliases[primary];
        }
        // end aliases

        if(primary.compare("listAvailable", Qt::CaseInsensitive) == 0)
        {
            //listAvailableAddresses();
        }
        else if(primary.compare("listen", Qt::CaseInsensitive) == 0)
        {
            /*
            if(server1.isListening())
            {
                qPrint(qout, tr("Server is already running."));
                return false;
            }
            else
            {
                if(commandParts.length() < 3)
                {
                    qPrint(qout, tr("Usage: listen [ip] [port]"));
                    return false;
                }
                else
                {
                    QHostAddress address = QHostAddress(commandParts[1]);
                    if(address.isNull())
                    {
                        qPrint(qout, tr("Ip isn't valid"));
                        return false;
                    }
                    bool ok;
                    int port = commandParts[2].toInt(&ok);
                    if(!ok)
                    {
                        qPrint(qout, tr("Port isn't int"));
                        return false;
                    }
                    if (server1.listen(address, port)) {
                        QString msg = tr("Server is listening on address %1 and port %2")
                                .arg(address.toString())
                                .arg(port);
                        qPrint(qout, msg);
                        addInfoMessage(msg);
                        return true;
                    }
                }
            }
            */
        }
        else if (primary.compare("unlisten", Qt::CaseInsensitive) == 0)
        {
            /*
            if(!server1.isListening())
            {
                qPrint(qout, tr("Server isn't running."));
                return false;
            }
            else
            {
                server1.close();
                qPrint(qout, tr("Server is not accepting new connections"));
                addInfoMessage(tr("Server is not accepting new connections"));
                return true;
            }
            */
        }
    }
    // emit invalidCommand();
    return false;
}

void ServerRun::invalidCommand()
{
    qPrint(qout, "Invalid Command, use 'showValid' for valid commands, 'help' for help, 'exit' to exit.");
}

void ServerRun::showAllCommands()
{
    qPrint(qout, "Use 'exit' to quit.");
    qPrint(qout, "");
    qPrint(qout, "All commands:");
    QStringList all;/*
    for(STCType c : STCType::_values())
    {
        all.append(c._to_string());
    }*/
    qls(all);
    qPrint(qout, "");
    qPrint(qout, "Use the following to change state:");
    QStringList states;/*
    for(STState s : STState::_values())
    {
        states.append(s._to_string());
    }
    qls(states);*/
    qPrint(qout, "");
}

template <class T>
void ServerRun::showCommands(const QList<T> valids)
{
    qPrint(qout, "Use 'exit' to quit.");
    qPrint(qout, "");
    qPrint(qout, "Available commands:");
    QStringList validList;
    typedef typename QList<T>::const_iterator Iter;
    for(Iter i = valids.constBegin(); i != valids.constEnd(); ++i)
    {
        validList.append(i->_to_string());
    }
    qls(validList);
    qPrint(qout, "");
    qPrint(qout, "Use the following to change state:");
    QStringList states;/*
    for(STState s : STState::_values())
    {
        states.append(s._to_string());
    }*/
    qls(states);
    qPrint(qout, "");
}

void ServerRun::showHelp(QStringList parameters)
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
int ServerRun::getConsoleWidth()
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
 * TBD: Use QFontMetrics to handle
 * https://stackoverflow.com/questions/31732698/getting-font-metrics-without-gui-console-mode
 * TBD: Qt Console can't display unicode correctly
 * https://bugreports.qt.io/browse/QTCREATORBUG-8099
 */
template<class T>
void ServerRun::qls(const QList<T> input)
{
    if(input.length() <= 0)
    {
        return;
    }
    int width = getConsoleWidth();

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

inline int ServerRun::callength(const QString &input, bool naive)
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

inline int ServerRun::callength(const QHostAddress &input, bool naive)
{
    Q_UNUSED(naive)
    return input.toString().length();
}

inline const QString & ServerRun::strfiy(const QString &in)
{
    return in;
}

inline QString ServerRun::strfiy(const QHostAddress &in)
{
    QString result = in.toString();
    return result;
}

void ServerRun::customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QString dt = QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm:ss");
    QString txt = QString("[%1] ").arg(dt);
    QByteArray localMsg = msg.toUtf8();
    const char *file = context.file ? context.file : "";
    const char *function = context.function ? context.function : "";

    QString txt2 = QString("%1 (%2:%3, %4)").arg(tr(localMsg.constData()), file, QString::number(context.line), function);
    switch (type)
    {
    case QtDebugMsg:
        txt += QString("{Debug} \t\t %1").arg(txt2);
        break;
    case QtInfoMsg:
        txt += QString("{Info} \t\t %1").arg(txt2);
        break;
    case QtWarningMsg:
        txt += QString("{Warning} \t %1").arg(txt2);
        break;
    case QtCriticalMsg:
        txt += QString("{Critical} \t %1").arg(txt2);

#ifdef Q_OS_WIN
        WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE),
                      txt.utf16(), txt.size(), NULL, NULL);
#else
        std::cout << txt.toUtf8().constData();
#endif
        std::cout << std::endl;

        break;
    case QtFatalMsg:
        txt += QString("{Fatal} \t\t %1").arg(txt2);

#ifdef Q_OS_WIN
        WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE),
                      txt.utf16(), txt.size(), NULL, NULL);
#else
        std::cout << txt.toUtf8().constData();
#endif
        std::cout << std::endl;

        abort();
        break;
    }

    QFile outFile("LogFile.log");
    outFile.open(QIODevice::WriteOnly | QIODevice::Append);

    QTextStream textStream(&outFile);
    textStream << txt << Qt::endl;
}
