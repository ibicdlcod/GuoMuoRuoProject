#ifndef COMMANDLINE_H
#define COMMANDLINE_H

#include <QCoreApplication>
#include <QFile>
#include <QTimer>

#include "consoletextstream.h"

class CommandLine : public QCoreApplication {
    Q_OBJECT

public:
    explicit CommandLine(int, char **);
    virtual ~CommandLine() noexcept = default;

    enum Password{
        normal,
        login,
        registering,
        confirm
    };
    Q_ENUM(Password);

public slots:
    static void customMessageHandler(QtMsgType,
                                     const QMessageLogContext &,
                                     const QString &);
    virtual void displayPrompt() = 0;
    void openingwords();
    bool parse(const QString &);
    virtual bool parseSpec(const QStringList &) = 0;
    void showHelp(const QStringList &);
    static const QStringList getCommands();

protected:
    virtual const QStringList getCommandsSpec() const = 0;
    virtual const QStringList getValidCommands() const = 0;
    void invalidCommand();
    void showCommands(bool);
    /* utility */
    static int callength(const QString &, bool naive = false);
    int getConsoleWidth();
    void qls(const QStringList &);
    /* exit */
    void exitGracefully();
    virtual void exitGraceSpec() = 0;

    ConsoleTextStream qout;
    Password passwordMode;
};

#endif // COMMANDLINE_H
