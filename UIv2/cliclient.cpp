#include "cliclient.h"

CliClient::CliClient(int argc, char ** argv)
    : CLI(argc, argv), client(nullptr)
{

}

void CliClient::update()
{
    /* With the NEW marvelous design, this function doesn't seem necessary. */
    //timer->start(1000); //reset the timer
    QCoreApplication::processEvents();
    QCoreApplication::processEvents();
    qout.flush();
}

bool CliClient::parseSpec(const QStringList &commandParts)
{
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

        if(primary.compare("start", Qt::CaseInsensitive) == 0)
        {
            if(commandParts.length() < 3)
            {
                qout << tr("Usage: start [ip] [port]") << Qt::endl;
                return true; // if false, then the above message and invalidCommand becomes redundant
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
                client->start("Client/debug/Client",
                              {commandParts[1], commandParts[2]}, QIODevice::ReadWrite);
                return true;
            }
        }
        else if(primary.compare("stop", Qt::CaseInsensitive) == 0)
        {
            shutdownClient();
            return true;
        }
        else if(primary.compare("unlisten", Qt::CaseInsensitive) == 0 && client && client->state()) // QProcess::NotRunning = 0
        {
            qout << tr("Client will stop accepting new connections.") << Qt::endl;
            client->write("UNLISTEN\n");
            return true;
        }
        else if(primary.compare("relisten", Qt::CaseInsensitive) == 0 && client && client->state())
        {
            if(commandParts.length() < 3)
            {
                qout << tr("Usage: relisten [ip] [port]") << Qt::endl;
                return true; // if false, then the above message and invalidCommand becomes redundant
            }
            qout << tr("Client will resume accepting new connections.") << Qt::endl;
            /* very ugly, but write() don't accept QString */
            char command[120];
            sprintf(command, "RELISTEN %s %s\n", commandParts[1].toUtf8().constData(), commandParts[2].toUtf8().constData());
            client->write(command);
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
        case 'E': qCritical("%s", output.constData()); break;
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

void CliClient::clientStarted()
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
    if(client && client->state()) // QProcess::NotRunning = 0
    {
        client->write("SIGTERM\n");
        qout << tr("Waiting for client finish...") << Qt::endl;
        //client->terminate();
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
