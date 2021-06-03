#include <iostream>
//#include "stdafx.h"
#include "qconsolelistener.h"

QConsoleListener::QConsoleListener(bool consolemode)
    : consolemode(consolemode)
{
    QObject::connect(
                this, &QConsoleListener::finishedGetLine,
                this, &QConsoleListener::on_finishedGetLine,
                Qt::QueuedConnection
                );
#ifdef Q_OS_WIN
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
#ifdef Q_OS_WIN
    QObject::connect(m_notifier, &QWinEventNotifier::activated,
                 #else
    QObject::connect(m_notifier, &QSocketNotifier::activated,
                 #endif
                     [this]() {
        /* the following is different from the original at https://github.com/juangburgos/QConsoleListener/blob/master/src/qconsolelistener.cpp
         * becase windows console can't handle unicode
         */
        QString res;
#ifdef Q_OS_WIN
        /*
        HWND consoleWnd = GetConsoleWindow();
        DWORD dwProcessId;
        GetWindowThreadProcessId(consoleWnd, &dwProcessId);
        qCritical() << GetCurrentProcessId() << std::endl;
        qCritical() << dwProcessId << std::endl;
        if (GetCurrentProcessId()==dwProcessId)
        */
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
    delete this;
}
