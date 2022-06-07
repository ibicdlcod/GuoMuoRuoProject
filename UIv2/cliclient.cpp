#include "cliclient.h"

#include <QSettings>
#include <QPasswordDigestor>

#include "ecma48.h"
#include "magic.h"

extern QSettings *settings;

CliClient::CliClient(int argc, char ** argv)
    : CLI(argc, argv), client(nullptr)
{

}

void CliClient::update()
{
    QCoreApplication::processEvents();
    QCoreApplication::processEvents();
    qout.flush();
}

void CliClient::displayPrompt()
{
    qout << "ST$ ";
}

bool CliClient::parseSpec(const QStringList &cmdParts)
{
    if(cmdParts.length() > 0)
    {
        QString primary = cmdParts[0];

        /* aliases */
        QMap<QString, QString> aliases;

        if(aliases.contains(primary))
        {
            primary = aliases[primary];
        }
        /* end aliases */

        if(primary.compare("connect", Qt::CaseInsensitive) == 0)
        {
            if(cmdParts.length() < 3)
            {
                qout << tr("Usage: connect [ip] [port]") << Qt::endl;
                /* if false, then the above message and invalidCommand becomes redundant */
                return true;
            }
            else
            {
                if(client && client->state())
                {
                    qout << tr("Client already exists, please shut down first.") << Qt::endl;
                    return true;
                }
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
                QString client_exe = QStringLiteral("../Client/debug/Client");
#elif defined(__GNUC__)
                QString client_exe = QStringLiteral("../Client/Client");
#elif defined (_MSC_VER)
                QString client_exe = QStringLiteral("../Client/debug/Client");
#endif
                if(settings->contains("Client location"))
                {
                    client_exe = settings->value("Client location").toString();
                }
                client->start(client_exe,
                              {cmdParts[1], cmdParts[2]}, QIODevice::ReadWrite);
                client->setReadChannel(QProcess::StandardOutput);
                return true;
            }
        }
        else if(primary.compare("disconnect", Qt::CaseInsensitive) == 0)
        {
            shutdownClient();
            return true;
        }
        else if(primary.compare("register", Qt::CaseInsensitive) == 0)
        {
            if(cmdParts.length() < 3 || cmdParts[2].size() < 8)
            {
                qout << tr("Usage: register [username] [password, >8 chars]") << Qt::endl;
                /* if false, then the above message and invalidCommand becomes redundant */
                return true;
            }
            else if(client && client->state())
            {
                QString name = cmdParts[1];
                QString password = cmdParts[2];
#pragma message(SALT_FISH)
                QByteArray salt = name.toUtf8().append(
                            settings->value("salt",
                                            "\xe8\xbf\x99\xe6\x98\xaf\xe4\xb8\x80\xe6\x9d\xa1\xe5\x92\xb8\xe9\xb1\xbc").toByteArray());
                QByteArray shadow = QPasswordDigestor::deriveKeyPbkdf2(QCryptographicHash::Blake2s_256,
                                                                       password.toUtf8(), salt, 8, 255);
                client->write(("REG " + cmdParts[1].toUtf8() + " " + QString(shadow.toHex()).toLatin1() + "\n"));
                return true;
            }
            else
            {
                qout << tr("Client isn't running, please connect first.");
                return true;
            }
        }
        else if(primary.compare("login", Qt::CaseInsensitive) == 0)
        {
            if(cmdParts.length() < 3 || cmdParts[2].size() < 8)
            {
                qout << tr("Usage: login [username] [password, >8 chars]") << Qt::endl;
                /* if false, then the above message and invalidCommand becomes redundant */
                return true;
            }
            else if(client && client->state())
            {
                QString name = cmdParts[1];
                QString password = cmdParts[2];
#pragma message(SALT_FISH)
                QByteArray salt = name.toUtf8().append(
                            settings->value("salt",
                                            "\xe8\xbf\x99\xe6\x98\xaf\xe4\xb8\x80\xe6\x9d\xa1\xe5\x92\xb8\xe9\xb1\xbc").toByteArray());
                QByteArray shadow = QPasswordDigestor::deriveKeyPbkdf2(QCryptographicHash::Blake2s_256,
                                                                       password.toUtf8(), salt, 8, 255);
                client->write(("LOGIN " + cmdParts[1].toUtf8() + " " + QString(shadow.toHex()).toLatin1() + "\n"));
                return true;
            }
            else
            {
                qout << tr("Client isn't running, please connect first.");
                return true;
            }
        }
        else if(primary.compare("logout", Qt::CaseInsensitive) == 0)
        {
            if(client && client->state())
            {
                client->write("LOGOUT\n");
                return true;
            }
            else
            {
                qout << tr("Client isn't running.");
                return true;
            }
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
    QList<QByteArray> outputs = client->readAllStandardError().split('\n');
    for(qsizetype i=0; i<outputs.size(); ++i)
    {
        QByteArray output = outputs.at(i).trimmed();
        if(output.simplified().size() < 1)
            continue;
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
                    qout.printLine(QStringLiteral("%1 %2 %3 %4 %5 %6")
                                   .arg(tr("[CATBOMB] TURKEY TROTS TO WATER"),
                                        tr("GG"),
                                        tr("FROM CINCPAC ACTION COM THIRD FLEET INFO COMINCH CTF SEVENTY-SEVEN X"),
                                        tr("WHERE IS RPT WHERE IS TASK FORCE THIRTY FOUR"),
                                        tr("RR"),
                                        tr("THE WORLD WONDERS")),
                                   Ecma(255,128,192), Ecma(255,255,255,true));
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
}

void CliClient::clientStdout()
{
    QByteArray output = client->readAllStandardOutput();
    qInfo("%s", output.constData());
}

inline void CliClient::clientStarted()
{
    /* deprecated */
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
    int waitformsec = settings->value("Client shutdown wait time", 12000).toInt();
    /* QProcess::NotRunning = 0 (this comment won't be repeated elsewhere) */
    if(client && client->state())
    {
        client->write("SIGTERM\n");
        qout << tr("Waiting for client finish...") << Qt::endl;
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
    result.append({"disconnect", "connect"});
    result.sort(Qt::CaseInsensitive);
    return result;
}

const QStringList CliClient::getValidCommands()
{
    QStringList result = QStringList();
    result.append(getCommands());
    if(client && client->state())
    {
        result.append("disconnect");
    }
    else
    {
        result.append("connect");
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
