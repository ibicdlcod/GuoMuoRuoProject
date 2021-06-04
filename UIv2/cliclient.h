#ifndef CLICLIENT_H
#define CLICLIENT_H

#include "cli.h"
#include <QProcess>

class CliClient : public CLI
{
public:
    explicit CliClient(int, char **);

public slots:
    void update();

private slots:
    bool parseSpec(const QStringList &);

    void processError(QProcess::ProcessError);
    void processFinished(int, QProcess::ExitStatus);
    void clientStderr();
    void clientStdout();
    void clientStarted();
    void clientChanged(QProcess::ProcessState);
    void shutdownClient();

private:
    const QStringList getCommandsSpec();
    const QStringList getValidCommands();
    void exitGraceSpec();

    QProcess *client;
};

#endif // CLICLIENT_H
