#ifndef COMMANDLINE_H
#define COMMANDLINE_H

#include <QCoreApplication>
#include <QFile>
#include <QTimer>

#include "consoletextstream.h"

class CommandLine : public QCoreApplication
{
    Q_OBJECT

public:
    explicit CommandLine(int, char **);

public slots:
    static void customMessageHandler(QtMsgType,
                                     const QMessageLogContext &,
                                     const QString &);
    void openingwords();
    bool parse(const QString &);
    virtual bool parseSpec(const QStringList &) = 0;
    virtual void displayPrompt() = 0;

protected:
    static const QStringList getCommands();
    virtual const QStringList getCommandsSpec() = 0;
    virtual const QStringList getValidCommands() = 0;
    void invalidCommand();
    void showCommands(bool);
    void showHelp(const QStringList &);

    static int callength(const QString &, bool naive = false);
    int getConsoleWidth();
    void qls(const QStringList &);

    void exitGracefully();
    virtual void exitGraceSpec() = 0;

    QTimer *timer;
    ConsoleTextStream qout;
    enum password
    {
        normal,
        login,
        registering,
        confirm
    };
    password passwordMode;
};

#endif // COMMANDLINE_H
