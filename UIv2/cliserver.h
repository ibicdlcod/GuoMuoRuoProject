#ifndef CLISERVER_H
#define CLISERVER_H

#include "cli.h"
#include <QProcess>

class CliServer : public CLI
{
public:
    explicit CliServer(int, char **);

public slots:
    void update();

private slots:
    bool parseSpec(const QStringList &);

    void processError(QProcess::ProcessError);
    void processFinished(int, QProcess::ExitStatus);
    void serverStderr();
    void serverStdout();
    void serverStarted();
    void serverChanged(QProcess::ProcessState);
    void shutdownServer();

private:
    const QStringList getCommandsSpec();
    const QStringList getValidCommands();
    void exitGraceSpec();

    QProcess *server;
};

#endif // CLISERVER_H
