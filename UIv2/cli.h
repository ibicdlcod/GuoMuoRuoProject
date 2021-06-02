#ifndef CLI_H
#define CLI_H

#include <QCoreApplication>
#include <QFile>
#include <QTimer>

#include "consoletextstream.h"

class CLI : public QCoreApplication
{
public:
    explicit CLI(int, char **);

public slots:
    static void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);
    void openingwords();
    bool parse(const QString &);
    virtual bool parseSpec(const QStringList &) = 0;

    void displayPrompt(); // TO BE VIRTUAL

protected:
    static const QStringList getCommands();
    virtual const QStringList getCommandsSpec() = 0;
    virtual const QStringList getValidCommands() = 0;
    void invalidCommand();
    void showCommands(bool validOnly);
    void showHelp(const QStringList &);

    static int callength(const QString &, bool naive = false);
    int getConsoleWidth();
    void qls(const QStringList &);

    void exitGracefully();
    virtual void exitGraceSpec() = 0;

    QTimer *timer;
    ConsoleTextStream qout;
};

#endif // CLI_H
