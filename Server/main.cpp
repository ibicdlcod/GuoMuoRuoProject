#include <QCoreApplication>
#include <QLocale>
#include <QTranslator>

#include <QTextStream>
#include <QtDebug>

#include <QConsoleListener>

#include "dtlsserver.h"
#include "messagehandler.h"

QConsoleListener *console;
int main(int argc, char *argv[])
{
    QT_USE_NAMESPACE

    QCoreApplication a(argc, argv);

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "Server_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }

    QStringList argv_l = QStringList();
    for(int i = 0; i < argc; ++i)
    {
        argv_l.append(argv[i]);
    }
    DtlsServer server;
    MessageHandler handler;

    bool success = QObject::connect(&server, &DtlsServer::errorMessage,
                                    &handler, &MessageHandler::errorMessage)
            && QObject::connect(&server, &DtlsServer::warningMessage,
                                &handler, &MessageHandler::warningMessage)
            && QObject::connect(&server, &DtlsServer::infoMessage,
                                &handler, &MessageHandler::infoMessage)
            && QObject::connect(&server, &DtlsServer::datagramReceived,
                                &handler, &MessageHandler::addClientMessage);
    if(!success)
    {
        qFatal("[Fatal] %s", server.tr("Communication with message handler can't be established.").toUtf8().constData());
    }

    QHostAddress address = QHostAddress(argv[1]);
    if(address.isNull())
    {
        emit server.errorMessage("Ip isn't valid");
        return 102;
    }
    unsigned int port = QString(argv[2]).toInt();
    if(port < 1024 || port > 49151)
    {
        emit server.errorMessage("Port isn't valid");
        return 103;
    }
    if (server.listen(address, port)) {
        QString msg = server.tr("Server is listening on address %1 and port %2")
                .arg(address.toString())
                .arg(port);
        emit server.infoMessage(msg);
        console = new QConsoleListener(false);
        bool success = QObject::connect(console, &QConsoleListener::newLine, &server, &DtlsServer::parse);
        bool success1 = QObject::connect(&server, &DtlsServer::finished, console, &QConsoleListener::exit);
        bool success0 = QObject::connect(&server, &DtlsServer::finished, &a, &QCoreApplication::quit, Qt::QueuedConnection);
        if(!success0 || !success1)
        {
            throw std::runtime_error("Exit mechanism failed!");
        }
        if(!success)
        {
            throw std::runtime_error("Connection with input parser failed!");
        }
        return a.exec();
    }
    else
    {
        QString msg = server.tr("Server failed to listen on address %1 and port %2")
                .arg(address.toString())
                .arg(port);
        emit server.errorMessage(msg);
        return 104;
    }
}
