#ifndef SERVERRUN_H
#define SERVERRUN_H

#include <QObject>

#include <QHostAddress>
#include <QProcess>
#include <QTimer>

#include "consoletextstream.h"
#include "wcwidth.h"
#include "qprint.h"

class ServerRun : public QObject
{
    Q_OBJECT

public:
    explicit ServerRun(QObject *parent = nullptr);
    static void customMessageHandler(QtMsgType, const QMessageLogContext &, const QString &);

signals:
    void finished();
    //void exit(int);

public slots:
    void run();
    void update();

private slots:
    void processError(QProcess::ProcessError);
    void processFinished(int, QProcess::ExitStatus);
    void serverStderr();
    void serverStdout();
    void serverStarted();
    void serverChanged(QProcess::ProcessState);
    void shutdownServer();
    bool parse(QString);

private:
    void invalidCommand();
    void showAllCommands();
    template<class T>
    void showCommands(const QList<T>);
    void showHelp(QStringList);

    int getConsoleWidth();
    template<class T>
    void qls(const QList<T>);
    static int callength(const QString &, bool naive = false);
    static int callength(const QHostAddress &, bool naive = false);
    static const QString & strfiy(const QString &);
    static QString strfiy(const QHostAddress &);
    void exitGracefully();

    ConsoleTextStream qout;
    ConsoleInput qin;
    QList<QHostAddress> availableAddresses;
    QProcess *server;
    QTimer *timer;
    bool readyToQuit;
};

#endif // SERVERRUN_H
