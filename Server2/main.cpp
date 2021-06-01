#include <QCoreApplication>
#include <QTextStream>

#include "dtlsserver.h"

int main(int argc, char *argv[])
{
    QT_USE_NAMESPACE

    QCoreApplication a(argc, argv);
    QTextStream qout(stdout);
    QTextStream qerr(stderr);

    DtlsServer server;
    //server.listen(QHostAddress("192.168.0.3"), 1826);

    if(argc < 3)
    {
        qerr << tr("Usage: (exe) [ip] [port]") << Qt::endl;
        return 1;
    }
    else
    {

        QHostAddress address = QHostAddress(argv[1]);
        if(address.isNull())
        {
            qerr << tr("Ip isn't valid") << Qt::endl;
            return 2;
        }
        unsigned int port = atoi(argv[2]);
        if(port < 1024 || port > 49151)
        {
            qerr << tr("Port isn't valid") << Qt::endl;
            return 3;
        }
        if (server.listen(address, port)) {
            QString msg = tr("Server is listening on address %1 and port %2")
                    .arg(address.toString())
                    .arg(port);
            qout << msg;
            emit server.infoMessage(msg);
            return 4;
        }
    }


    return a.exec();
}
