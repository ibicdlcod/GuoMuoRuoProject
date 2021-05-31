#ifndef RUN_H
#define RUN_H

#include <QObject>
#include <QHostAddress>

#include "consoletextstream.h"
#include "wcwidth.h"
#include "qprint.h"

class Run : public QObject
{
    Q_OBJECT

public:
    explicit Run(QObject *parent = nullptr);
    static void customMessageHandler(QtMsgType, const QMessageLogContext &, const QString &);

signals:
    void finished();

private slots:
    void run();

    void invalidCommand();
    void showAllCommands();
    void showCommands(const QList<STCType>);
    void showHelp(QStringList);

private:
    int getConsoleWidth();
    template<class T>
    void qls(const QList<T>);
    static int callength(const QString &, bool naive = false);
    static int callength(const QHostAddress &, bool naive = false);
    static const QString & strfiy(const QString &);
    static QString strfiy(const QHostAddress &);
    bool parse(QString);
    void exitGracefully();

    ConsoleTextStream qout;
    ConsoleInput qin;
    QList<QHostAddress> availableAddresses;
};

#endif // RUN_H
