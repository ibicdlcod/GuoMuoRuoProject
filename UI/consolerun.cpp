#include "consolerun.h"
#include "qprint.h"
#include "ecma48.h"

#include <QDir>

ConsoleRun::ConsoleRun(QObject *parent)
    : QObject(parent), readyToQuit(false),
      qout(ConsoleTextStream()), qin(ConsoleInput())
{
    logFile = new QFile("LogFile.log");
    logFile->open(QIODevice::WriteOnly | QIODevice::Append); // Where is close()?
}

ConsoleRun::~ConsoleRun()
{
    logFile->close();
}

void ConsoleRun::run()
{
    /*
    int width = getConsoleWidth();
    QString notice;
    QDir serverDir = QDir::current();
    QFile openingwords(serverDir.filePath("openingwords.txt"));
    if(!openingwords.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qPrint(qout, tr("Opening words file not found, exiting."));
        emit finished();
        return;
    }
    else
    {
        QTextStream instream1(&openingwords);
        qout << Ecma48(255,255,255,true) << Ecma48(0,0,255);
        qout.setFieldAlignment(QTextStream::AlignCenter);
        while(!instream1.atEnd())
        {
            notice = instream1.readLine();
            qout.setFieldWidth(width);
            qout << notice;
            qout.setFieldWidth(0);
            qout << Qt::endl;
        }
        qout << Qt::endl;
        qout << Ecma48(192,255,192,true) << Ecma48(64,64,64);

        qout.setFieldWidth(width);
        qout << tr("What? Admiral Tanaka? He's the real deal, isn't he? Great at battle and bad at politics--so cool!");
        qout.setFieldWidth(0);
        qout << Qt::endl;
    }
    qout << EcmaSetter::AllDefault;
    qout.setFieldAlignment(QTextStream::AlignLeft);
    qout << Qt::endl;

    timer = new QTimer(this);
    QObject::connect(timer, &QTimer::timeout, this, &ConsoleRun::update);
    QObject::connect(&qin, &ConsoleInput::textReceived, this, &ConsoleRun::parse);
    timer->start(1000);

    while(!readyToQuit)
    {
        update();
        qout << "STS ";
        //qout << ((server && server->isWritable()) ? "RUNNING" : "NOTRUNNING");
        qout << "$ ";
        qin.readline();
    }
    return;
    */
}
