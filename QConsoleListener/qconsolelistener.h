#ifndef QCONSOLELISTENER_H
#define QCONSOLELISTENER_H

#pragma once

#include <QObject>
#include <QThread>
#include <iostream>

#if defined (Q_OS_WIN)
#include <QWinEventNotifier>
#include <windows.h>
#else
#include <QSocketNotifier>
#endif

class QConsoleListener : public QObject
{
    Q_OBJECT

public:
    QConsoleListener(bool);
    ~QConsoleListener();

Q_SIGNALS:
    // connect to "newLine" to receive console input
    void newLine(const QString &strNewLine);
    // finishedGetLine if for internal use
    void finishedGetLine(const QString &strNewLine);

private:
#if defined (Q_OS_WIN)
    QWinEventNotifier *m_notifier;
#else
    QSocketNotifier *m_notifier;
#endif

public Q_SLOTS:
    void exit();

private Q_SLOTS:
    void on_finishedGetLine(const QString &strNewLine);

private:
    QThread m_thread;
    bool consolemode;
};

#endif // QCONSOLELISTENER_H
