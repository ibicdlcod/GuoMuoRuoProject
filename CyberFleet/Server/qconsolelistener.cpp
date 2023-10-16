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
#include <iostream>
#include "qconsolelistener.h"

QConsoleListener::QConsoleListener(bool consolemode)
    : consolemode(consolemode)
{
    QObject::connect(
                this, &QConsoleListener::finishedGetLine,
                this, &QConsoleListener::on_finishedGetLine,
                Qt::QueuedConnection
                );
#if defined (Q_OS_WIN)
    m_notifier = new QWinEventNotifier(GetStdHandle(STD_INPUT_HANDLE));
#else
    m_notifier = new QSocketNotifier(fileno(stdin), QSocketNotifier::Read);
#endif
    // NOTE : move to thread because std::getline blocks,
    //        then we sync with main thread using a QueuedConnection with finishedGetLine
    m_notifier->moveToThread(&m_thread);
    QObject::connect(
                &m_thread , &QThread::finished,
                m_notifier, &QObject::deleteLater
                );
#if defined (Q_OS_WIN)
    QObject::connect(m_notifier, &QWinEventNotifier::activated,
#else
    QObject::connect(m_notifier, &QSocketNotifier::activated,
#endif
                     [this]() {
        /* the following is different from the original at https://github.com/juangburgos/QConsoleListener/blob/master/src/qconsolelistener.cpp
         * becase windows console can't handle unicode
         */
        QString res;
#if defined (Q_OS_WIN)
        if(this->consolemode)
        {
            const int bufsize = 512;
            wchar_t buf[bufsize];
            DWORD read;
            do {
                ReadConsoleW(GetStdHandle(STD_INPUT_HANDLE),
                             buf, bufsize, &read, NULL);
                res += QString::fromWCharArray(buf, read);
            } while (read > 0 && res[res.length() - 1] != '\n');
            // could just do res.truncate(res.length() - 2), but better be safe
            while (res.length() > 0
                   && (res[res.length() - 1] == '\r' || res[res.length() - 1] == '\n'))
                res.truncate(res.length() - 1);
        }
        else
        {
            std::string line;
            std::getline(std::cin, line);
            res = QString::fromStdString(line);
        }
#else
        std::string line;
        std::getline(std::cin, line);
        res = QString::fromStdString(line);
#endif
        Q_EMIT this->finishedGetLine(res);
    });
    m_thread.start();
}

void QConsoleListener::on_finishedGetLine(const QString &strNewLine)
{
    Q_EMIT this->newLine(strNewLine);
}

QConsoleListener::~QConsoleListener()
{
    m_thread.quit();
    m_thread.wait();
}

void QConsoleListener::exit()
{
    /* fuck my life, different COMPILERS (as opposed to operating systems) behaves differently */
#if defined(_MSC_VER)
    /* apparently nothing to do */
#endif
#if defined (__MINGW32__) || defined (__MINGW64__)
    free(this);
#elif defined(__GNUC__) /* beware that mingw defines __GNUC__ */
    delete this;
#endif
}

void QConsoleListener::turnOffEchoing()
{
#if defined(Q_OS_WIN)
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hStdin, &mode);
    SetConsoleMode(hStdin, mode & (~ENABLE_ECHO_INPUT));
#endif
#if defined(Q_OS_UNIX)
    termios oldt;
    tcgetattr(STDIN_FILENO, &oldt);
    termios newt = oldt;
    newt.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
#endif
}

void QConsoleListener::turnOnEchoing()
{
#if defined(Q_OS_WIN)
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hStdin, &mode);
    SetConsoleMode(hStdin, mode | (ENABLE_ECHO_INPUT));
#endif
#if defined(Q_OS_UNIX)
    termios oldt;
    tcgetattr(STDIN_FILENO, &oldt);
    termios newt = oldt;
    newt.c_lflag |= ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
#endif
}
