#include "run.h"

#include <QFile>
#include <QDir>
#include <QDebug>

#include "qprint.h"
#include "ecma48.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif
#ifdef Q_OS_UNIX
#include <sys/ioctl.h> //ioctl() and TIOCGWINSZ
#include <unistd.h> // for STDOUT_FILENO
#endif

Run::Run(QObject *parent)
    : QObject(parent), qout(ConsoleTextStream())
{
    connect(&server, &DtlsServer::errorMessage, this, &Run::addErrorMessage);
    connect(&server, &DtlsServer::warningMessage, this, &Run::addWarningMessage);
    connect(&server, &DtlsServer::infoMessage, this, &Run::addInfoMessage);
    connect(&server, &DtlsServer::datagramReceived, this, &Run::addClientMessage);
}

void Run::run()
{
    int width;
#ifdef Q_OS_WIN
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
#else
#ifdef Q_OS_UNIX
    struct winsize size;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
    width = size.ws_col;
#else
    width = 80;
#endif
#endif

    QString notice;
    QDir serverDir = QDir::current();
    QFile openingwords(serverDir.filePath("openingwords.txt"));
    if(!openingwords.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qPrint(qout, "Opening words file not found, exiting.");
        emit finished();
        return;
    }
    else
    {
        QTextStream instream1(&openingwords);
        qout.setFieldAlignment(QTextStream::AlignCenter);
        qout << Ecma48(255,255,255,true) << Ecma48(0,0,255);
        while(!instream1.atEnd())
        {
            notice = instream1.readLine();
            qout << qSetFieldWidth(width) << notice << qSetFieldWidth(0) << Qt::endl;
        }
    }
    qout << Qt::endl;

    qout.setFieldAlignment(QTextStream::AlignLeft);
    qout << Ecma48(192,255,192,true) << Ecma48(0,0,0);
    qPrint(qout, "What? Admiral Tanaka? He's the real deal, isn't he? Great at battle and bad at politics--so cool!");
    qout << EcmaSetter::AllDefault;

    addErrorMessage("星薇");
    addWarningMessage("星薇");
    addInfoMessage("星薇");

    emit finished();
    return;
}

void Run::addErrorMessage(const QString &message)
{
    qout << Ecma48(192,0,0) << message << Qt::endl;
    qCritical() << message.toUtf8().constData();
}

void Run::addWarningMessage(const QString &message)
{
    qout << Ecma48(192,192,0) << message << Qt::endl;
    qWarning() << message.toUtf8().constData();
}

void Run::addInfoMessage(const QString &message)
{
    qout << Ecma48(0,192,0) << message << Qt::endl;
    qInfo() << message.toUtf8().constData();
}

void Run::addClientMessage(const QString &peerInfo, const QByteArray &datagram,
                                  const QByteArray &plainText)
{
    static const QString formatter = QStringLiteral("---------------\n"
                                                    "A message from %1\n"
                                                    "DTLS datagram: %2\n"
                                                    "As plain text: %3\n");
    const QString message = formatter.arg(peerInfo, QString::fromUtf8(datagram.toHex(' ')),
                                       QString::fromUtf8(plainText));
    qout << Ecma48(0,192,255) << message;
}

void Run::customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    //Q_UNUSED(context);

    QString dt = QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm:ss");
    QString txt = QString("[%1] ").arg(dt);
    QByteArray localMsg = msg.toUtf8();
    const char *file = context.file ? context.file : "";
    const char *function = context.function ? context.function : "";

    switch (type)
    {
    case QtDebugMsg:
        txt += QString("{Debug} \t\t %1 (%2:%3, %4)").arg(localMsg.constData()).arg(file).arg(context.line).arg(function);
        break;
    case QtInfoMsg:
        txt += QString("{Info} \t\t %1 (%2:%3, %4)").arg(localMsg.constData()).arg(file).arg(context.line).arg(function);
        break;
    case QtWarningMsg:
        txt += QString("{Warning} \t %1 (%2:%3, %4)").arg(localMsg.constData()).arg(file).arg(context.line).arg(function);
        break;
    case QtCriticalMsg:
        txt += QString("{Critical} \t %1 (%2:%3, %4)").arg(localMsg.constData()).arg(file).arg(context.line).arg(function);
        break;
    case QtFatalMsg:
        txt += QString("{Fatal} \t\t %1 (%2:%3, %4)").arg(localMsg.constData()).arg(file).arg(context.line).arg(function);
        abort();
        break;
    }

    QFile outFile("LogFile.log");
    outFile.open(QIODevice::WriteOnly | QIODevice::Append);

    QTextStream textStream(&outFile);
    textStream << txt << Qt::endl;
}
