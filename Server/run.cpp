#include "run.h"

Run::Run(QObject *parent, int argc, QStringList argv)
    : QObject(parent),
      argc(argc), argv(argv)
{

}

void Run::run()
{
    QTextStream qout = QTextStream(stdout);
    QTextStream qerr = QTextStream(stderr);

    DtlsServer server;
    MessageHandler handler;

    bool success = QObject::connect(&server, &DtlsServer::errorMessage,
                                    &handler, &MessageHandler::errorMessage)
            && QObject::connect(&server, &DtlsServer::warningMessage,
                                &handler, &MessageHandler::warningMessage)
            && QObject::connect(&server, &DtlsServer::infoMessage,
                                &handler, &MessageHandler::infoMessage);
    if(!success)
    {
        qFatal("[Fatal] %s", server.tr("Communication with message handler can't be established.").toUtf8().constData());
    }

    if(argc < 3)
    {
        emit server.infoMessage(tr("Usage: (exe) [ip] [port]"));
        emit exit(101);
    }
    else
    {

        QHostAddress address = QHostAddress(argv[1]);
        if(address.isNull())
        {
            emit server.errorMessage(tr("Ip isn't valid"));
            emit exit(102);
            return;
        }
        unsigned int port = argv[2].toInt();
        if(port < 1024 || port > 49151)
        {
            emit server.errorMessage(tr("Port isn't valid"));
            emit exit(103);
            return;
        }
        if (server.listen(address, port)) {
            QString msg = server.tr("Server is listening on address %1 and port %2")
                    .arg(address.toString())
                    .arg(port);
            emit server.infoMessage(msg);
        }
        else
        {
            QString msg = server.tr("Server failed to listen on address %1 and port %2")
                    .arg(address.toString())
                    .arg(port);
            emit server.errorMessage(msg);
            emit exit(104);
            return;
        }
    }
    QTextStream qin = QTextStream(stdin);
    while(true)
    {
        QString cmdline = qin.readLine();
        if(cmdline.startsWith("SIGTERM"))
        {
            server.close();
            emit server.infoMessage(tr("Server is not accepting new connections"));
            emit exit(0);
            return;
        }
    }
}
