#include "run.h"

#include <QFile>
#include <QDir>
#include <QDebug>

#include "qprint.h"

#include <cstdio>
#include <iostream>
//#include <stdio.h>
//#include <wchar.h>
#ifdef Q_OS_WIN
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#endif

Run::Run(QObject *parent)
    : QObject(parent)
{
    connect(&server, &DtlsServer::errorMessage, this, &Run::addErrorMessage);
    connect(&server, &DtlsServer::warningMessage, this, &Run::addWarningMessage);
    connect(&server, &DtlsServer::infoMessage, this, &Run::addInfoMessage);
    connect(&server, &DtlsServer::datagramReceived, this, &Run::addClientMessage);
}

void Run::run()
{
#ifdef Q_OS_WIN
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE)
    {
        throw GetLastError();
    }

    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode))
    {
        throw GetLastError();
    }

    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (!SetConsoleMode(hOut, dwMode))
    {
        throw GetLastError();
    }
#endif
    QTextStream qout = QTextStream(stdout);
    QString notice;
    QDir serverDir = QDir::current();
    QFile openingwords(serverDir.filePath("openingwords.txt"));
    if(!openingwords.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qprint(qout, "Opening words file not found, exiting.");
        emit finished();
        return;
    }
    else
    {
        QTextStream instream1(&openingwords);
        notice = instream1.readAll();
    }

    QString cmdline;

    qprint(qout, notice);
    qprint(qout, "Try some Set Graphics Rendition (SGR) terminal escape sequences");
    qprint(qout, "\x1b[31mThis text has a red foreground using SGR.31.");
    qprint(qout, "\x1b[1mThis text has a bright (bold) red foreground using SGR.1 to affect the previous color setting.");
    qprint(qout, "\x1b[mThis text has returned to default colors using SGR.0 implicitly.");
    qprint(qout, "\x1b[34;46mThis text shows the foreground and background change at the same time.");
    qprint(qout, "\x1b[0mThis text has returned to default colors using SGR.0 explicitly.");
    qprint(qout, "\x1b[31;32;33;34;35;36;101;102;103;104;105;106;107mThis text attempts to apply many colors in the same command. Note the colors are applied from left to right so only the right-most option of foreground cyan (SGR.36) and background bright white (SGR.107) is effective.");
    qprint(qout, "\x1b[49mThis text has restored the background color only.");
    qprint(qout, "\x1b[39mThis text has restored the foreground color only.");

    /*
    // Try some Set Graphics Rendition (SGR) terminal escape sequences
    wprintf(L"\x1b[31mThis text has a red foreground using SGR.31.\r\n");
    wprintf(L"\x1b[1mThis text has a bright (bold) red foreground using SGR.1 to affect the previous color setting.\r\n");
    wprintf(L"\x1b[mThis text has returned to default colors using SGR.0 implicitly.\r\n");
    wprintf(L"\x1b[34;46mThis text shows the foreground and background change at the same time.\r\n");
    wprintf(L"\x1b[0mThis text has returned to default colors using SGR.0 explicitly.\r\n");
    wprintf(L"\x1b[31;32;33;34;35;36;101;102;103;104;105;106;107mThis text attempts to apply many colors in the same command. Note the colors are applied from left to right so only the right-most option of foreground cyan (SGR.36) and background bright white (SGR.107) is effective.\r\n");
    wprintf(L"\x1b[39mThis text has restored the foreground color only.\r\n");
    wprintf(L"\x1b[49mThis text has restored the background color only.\r\n");*/

    emit finished();
    return;
}

void Run::addErrorMessage(const QString &message)
{

}

void Run::addWarningMessage(const QString &message)
{

}

void Run::addInfoMessage(const QString &message)
{

}

void Run::addClientMessage(const QString &peerInfo, const QByteArray &datagram,
                                  const QByteArray &plainText)
{

}
