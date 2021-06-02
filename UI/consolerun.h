#ifndef CONSOLERUN_H
#define CONSOLERUN_H

#include <QObject>
#include <QTimer>
#include <QHostAddress>
#include <QFile>

#include "consoletextstream.h"

class ConsoleRun : public QObject
{
    Q_OBJECT
public:
    explicit ConsoleRun(QObject *parent = nullptr);
    ~ConsoleRun();
    static void customMessageHandler(QtMsgType, const QMessageLogContext &, const QString &);

signals:
    void finished();

public slots:
    void run();
    //void update();

private slots:
    //virtual void displayprompt();
    //virtual bool parse(const QString &);

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

    //virtual void exitGracefully();

    QFile *logFile;
    bool readyToQuit;
    QTimer *timer;
    ConsoleTextStream qout;
    ConsoleInput qin;
};

#endif // CONSOLERUN_H
