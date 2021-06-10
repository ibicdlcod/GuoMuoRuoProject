#include "cliclient.h"
#include "ecma48.h"
#include "magic.h"

CliClient::CliClient(int argc, char ** argv)
    : CLI(argc, argv), client(nullptr)
{

}

void CliClient::update()
{
    /* With the NEW marvelous design, this function doesn't seem necessary. */
    QCoreApplication::processEvents();
    QCoreApplication::processEvents();
    qout.flush();
}

bool CliClient::parseSpec(const QStringList &commandParts)
{
    if(commandParts.length() > 0)
    {
        QString primary = commandParts[0];

        /* aliases */
        QMap<QString, QString> aliases;

        if(aliases.contains(primary))
        {
            primary = aliases[primary];
        }
        /* end aliases */

        if(primary.compare("login", Qt::CaseInsensitive) == 0)
        {
            if(commandParts.length() < 3)
            {
                qout << tr("Usage: login [ip] [port]") << Qt::endl;
                /* if false, then the above message and invalidCommand becomes redundant */
                return true;
            }
            else
            {
                client = new QProcess();
                bool success = QObject::connect(client, &QProcess::errorOccurred,
                                                this, &CliClient::processError)
                        && QObject::connect(client, &QProcess::started,
                                            this, &CliClient::clientStarted)
                        && QObject::connect(client, &QProcess::stateChanged,
                                            this, &CliClient::clientChanged)
                        && QObject::connect(client, &QProcess::finished,
                                            this, &CliClient::processFinished)
                        && QObject::connect(client, &QProcess::readyReadStandardError,
                                            this, &CliClient::clientStderr)
                        && QObject::connect(client, &QProcess::readyReadStandardOutput,
                                            this, &CliClient::clientStdout);
                if(!success)
                {
                    qFatal("Communication with client process can't be established.");
                }
#if defined (__MINGW32__) || defined (__MINGW64__)
                QString client_exe = QStringLiteral("mingw/Client/debug/Client");
#elif defined(__GNUC__)
                QString client_exe = QStringLiteral("gcc/debug/Client/Client");
#elif defined (_MSC_VER)
                QString client_exe = QStringLiteral("msvc/Client/debug/Client");
#endif
                client->start(client_exe,
                              {commandParts[1], commandParts[2]}, QIODevice::ReadWrite);
                return true;
            }
        }
        else if(primary.compare("logout", Qt::CaseInsensitive) == 0)
        {
            shutdownClient();
            return true;
        }
        return false;
    }
    return false;
}

void CliClient::processError(QProcess::ProcessError error)
{
    switch(error)
    {
    case QProcess::FailedToStart:
        qCritical("Client failed to start."); break;
    case QProcess::Crashed:
        qCritical("Client crashed."); break;
    case QProcess::Timedout:
        qCritical("Client function timed out."); break;
    case QProcess::WriteError:
        qCritical("Error writing to the client process."); break;
    case QProcess::ReadError:
        qCritical("Error reading from the client process."); break;
    case QProcess::UnknownError:
        qCritical("Client had unknown error."); break;
    }
}

void CliClient::processFinished(int exitcode, QProcess::ExitStatus exitst)
{
    switch(exitst)
    {
    case QProcess::NormalExit:
        qInfo("Client finished normally with exit code %d", exitcode);
        break;
    case QProcess::CrashExit:
        qCritical("Client crashed."); break;
    }
}


void CliClient::clientStderr()
{
    QByteArray output = client->readAllStandardError();
    const char * output_str = output.constData();
    if(output.startsWith("[Client"))
    {
        switch(output_str[7])
        {
        case 'E':
            if(output.startsWith("[ClientError] [CATBOMB]"))
            {
#pragma message(NOT_M_CONST)
                /* See https://en.wikipedia.org/w/index.php?title=The_world_wonders&oldid=1014651994 */
                qout.printLine(QStringLiteral("\r%1 %2 %3 %4 %5 %6")
                               .arg(tr("TURKEY TROTS TO WATER"),
                                    tr("GG"),
                                    tr("FROM CINCPAC ACTION COM THIRD FLEET INFO COMINCH CTF SEVENTY-SEVEN X"),
                                    tr("WHERE IS RPT WHERE IS TASK FORCE THIRTY FOUR"),
                                    tr("RR"),
                                    tr("THE WORLD WONDERS")),
                               Ecma(255,128,192), Ecma(255,255,255,true));
                qout.printLine("\r\n", Ecma(EcmaSetter::AllDefault));
            }
            else
            {
                qCritical("%s", output.constData());
            }
            break;
        case 'W': qWarning("%s", output.constData()); break;
        case 'I': qInfo("%s", output.constData()); break;
        default: qCritical("%s", output.constData()); break;
        }
    }
    else
    {
        qCritical("%s", output.constData());
    }
}

void CliClient::clientStdout()
{
    QByteArray output = client->readAllStandardOutput();
    qInfo("%s", output.constData());
}

inline void CliClient::clientStarted()
{
    qout << tr("Client started and running.") << Qt::endl;
}

void CliClient::clientChanged(QProcess::ProcessState newstate)
{
    switch(newstate)
    {
    case QProcess::NotRunning:
        qCritical("Client->NotRunning."); break;
    case QProcess::Starting:
        qInfo("Client->Starting."); break;
    case QProcess::Running:
        qInfo("Client->Running."); break;
    }
}

void CliClient::shutdownClient()
{
    int waitformsec = 12000;
    /* QProcess::NotRunning = 0 (this comment won't be repeated elsewhere */
    if(client && client->state())
    {
        client->write("SIGTERM\n");
        qout << tr("Waiting for client finish...") << Qt::endl;
        /* per documentation, this function is nearly useless on Windows
        client->terminate(); */
        if(!client->waitForFinished(waitformsec))
        {
            qout << (tr("Client isn't responding after %1 msecs, killing.")).arg(QString::number(waitformsec))
                 << Qt::endl;
            client->kill();
        }
    }
    else
    {
        qout << tr("Client isn't running.") << Qt::endl;
    }
}

const QStringList CliClient::getCommandsSpec()
{
    QStringList result = QStringList();
    result.append(getCommands());
    result.append({"login", "logout"});
    result.sort(Qt::CaseInsensitive);
    return result;
}

const QStringList CliClient::getValidCommands()
{
    QStringList result = QStringList();
    result.append(getCommands());
    if(client && client->state())
    {
        result.append("logout");
    }
    else
    {
        result.append("login");
    }
    result.sort(Qt::CaseInsensitive);
    return result;
}

void CliClient::exitGraceSpec()
{
    if(client && client->state())
    {
        shutdownClient();
    }
}
