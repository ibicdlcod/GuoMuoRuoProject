#include <QCoreApplication>
#include <QLocale>
#include <QTranslator>

#include <QTextStream>
#include <QtDebug>
#include <QSettings>

#include "qconsolelistener.h"

#include "dtlsclient.h"
#include "magic.h"

QSettings *settings;

int main(int argc, char *argv[])
{
    QT_USE_NAMESPACE

    QCoreApplication a(argc, argv);

    a.setApplicationName("SpearofTanaka");
    a.setApplicationVersion("0.0.0"); // temp
    a.setOrganizationName("Kantai Self-Governing Patriotic Committee");
    a.setOrganizationDomain("xxx.xyz"); // temp
    settings = new QSettings();

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "Client_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }

    /* clazy warning here can be ignored, it's written as if Qt 5 */
    QStringList argv_l = QStringList();

    for(int i = 0; i < argc; ++i)
    {
        argv_l.append(argv[i]);
    }
    try {
        QHostAddress address = QHostAddress(argv[1]);
        if(address.isNull())
        {
            qCritical() << "[ClientError] Ip isn't valid";
            return 102;
        }
        quint16 port = QString(argv[2]).toInt();
        if(port < 1024 || port > 49151)
        {
            qCritical() << "[ClientError] Port isn't valid";
            return 103;
        }
        /*
#pragma message(SALT_FISH)
        QByteArray salt = username.toUtf8().append(settings->value("salt",
                                          "\xe8\xbf\x99\xe6\x98\xaf\xe4\xb8\x80\xe6\x9d\xa1\xe5\x92\xb8\xe9\xb1\xbc").toByteArray());
        QByteArray shadow;
        if(password.startsWith("--register"))
        {
            shadow = "register";
        }
        else
        {
            shadow = QPasswordDigestor::deriveKeyPbkdf2(QCryptographicHash::Blake2s_256, password, salt, 8, 256);
        }
        */
#pragma message(M_CONST)
        DtlsClient client(address, port, "Alice Zephyr");
        QConsoleListener *console;
        console = new QConsoleListener(false);
        bool listenInput = QObject::connect(console, &QConsoleListener::newLine, &client, &DtlsClient::parse);
        bool unlistenInput = QObject::connect(&client, &DtlsClient::finished, console, &QConsoleListener::exit);
        bool exitMechanism = QObject::connect(&client, &DtlsClient::finished, &a, &QCoreApplication::quit, Qt::QueuedConnection);
        if(!listenInput)
        {
            throw std::runtime_error("Connection with input parser failed!");
        }
        if(!exitMechanism || !unlistenInput)
        {
            throw std::runtime_error("Exit mechanism failed!");
        }
        client.startHandshake();
        int result = a.exec();
        delete settings;
        return result;
    }  catch (std::runtime_error &e) {
        qDebug() << "[Runtime Error] " << e.what() << Qt::endl << "Press ENTER to exit.";
        return 2;
    }  catch (std::exception &e) {
        qDebug() << e.what() << Qt::endl << "Press ENTER to exit.";
        return -1;
    }
}
