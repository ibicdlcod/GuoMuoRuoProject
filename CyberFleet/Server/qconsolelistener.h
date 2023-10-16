/* MIT License

Copyright (c) 2018 Juan Gonzalez Burgos

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

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
#if defined (Q_OS_UNIX)
#include <termios.h>
#include <unistd.h>
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
    void turnOffEchoing();
    void turnOnEchoing();

private Q_SLOTS:
    void on_finishedGetLine(const QString &strNewLine);

private:
    QThread m_thread;
    bool consolemode;
};

#endif // QCONSOLELISTENER_H
