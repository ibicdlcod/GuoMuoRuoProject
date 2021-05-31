#include "run.h"

#include <QProcess>

#include "consoletextstream.h"
#include "ecma48.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif
#ifdef Q_OS_UNIX
#include <sys/ioctl.h> //ioctl() and TIOCGWINSZ
#include <unistd.h> // for STDOUT_FILENO
#endif

Run::Run(QObject *parent)
    : QObject(parent), qout(ConsoleTextStream()), qin(ConsoleInput())
{
    connect(&server1, &DtlsServer::errorMessage, this, &Run::addErrorMessage);
    connect(&server1, &DtlsServer::warningMessage, this, &Run::addWarningMessage);
    connect(&server1, &DtlsServer::infoMessage, this, &Run::addInfoMessage);
    connect(&server1, &DtlsServer::datagramReceived, this, &Run::addClientMessage);
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

    QString cmdline;
    while(true)
    {
        qout << "STS " + QString(server1.isListening() ? "ACTIVE" : "INACTIVE") + "$ ";
        cmdline = qin.readLine();

        if(cmdline.compare("start") == 0)
        {
            QProcess *server = new QProcess();
            server->start("STServerCore", {}, QIODevice::WriteOnly);
        }
        if(cmdline.compare("exit") == 0)
        {
            break;
        }
        parse(cmdline);

        //TBD: record in history
    }

    exitGracefully();
    return;
}

bool Run::parse(QString command)
{
    QStringList commandParts = command.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    if(commandParts.length() > 0)
    {
        QString primary = commandParts[0];

        // aliases
        QMap<QString, QString> aliases;
        aliases["li"] = "listen";
        aliases["uli"] = "unlisten";
        aliases["ls"] = "listAvailable";

        if(aliases.contains(primary))
        {
            primary = aliases[primary];
        }
        // end aliases

        if(primary.compare("listAvailable", Qt::CaseInsensitive) == 0)
        {
            listAvailableAddresses();
        }
        else if(primary.compare("listen", Qt::CaseInsensitive) == 0)
        {
            if(server1.isListening())
            {
                qPrint(qout, tr("Server is already running."));
                return false;
            }
            else
            {
                if(commandParts.length() < 3)
                {
                    qPrint(qout, tr("Usage: listen [ip] [port]"));
                    return false;
                }
                else
                {
                    QHostAddress address = QHostAddress(commandParts[1]);
                    if(address.isNull())
                    {
                        qPrint(qout, tr("Ip isn't valid"));
                        return false;
                    }
                    bool ok;
                    int port = commandParts[2].toInt(&ok);
                    if(!ok)
                    {
                        qPrint(qout, tr("Port isn't int"));
                        return false;
                    }
                    if (server1.listen(address, port)) {
                        QString msg = tr("Server is listening on address %1 and port %2")
                                .arg(address.toString())
                                .arg(port);
                        qPrint(qout, msg);
                        addInfoMessage(msg);
                        return true;
                    }
                }
            }
        }
        else if (primary.compare("unlisten", Qt::CaseInsensitive) == 0)
        {
            if(!server1.isListening())
            {
                qPrint(qout, tr("Server isn't running."));
                return false;
            }
            else
            {
                server1.close();
                qPrint(qout, tr("Server is not accepting new connections"));
                addInfoMessage(tr("Server is not accepting new connections"));
                return true;
            }
        }
    }
    // emit invalidCommand();
    return false;
}

void Run::exitGracefully()
{
    qout << EcmaSetter::AllDefault;
    qPrint(qout, tr("ST ended, press ENTER to quit"));
    emit finished();
}

inline void Run::addErrorMessage(const QString &message)
{
    qCritical() << message.toUtf8().constData();
}

inline void Run::addWarningMessage(const QString &message)
{
    qWarning() << message.toUtf8().constData();
}

inline void Run::addInfoMessage(const QString &message)
{
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
    QString dt = QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm:ss");
    QString txt = QString("[%1] ").arg(dt);
    QByteArray localMsg = msg.toUtf8();
    const char *file = context.file ? context.file : "";
    const char *function = context.function ? context.function : "";

    QString txt2 = QString("%1 (%2:%3, %4)").arg(localMsg.constData(), file, QString::number(context.line), function);
    switch (type)
    {
    case QtDebugMsg:
        txt += QString("{Debug} \t\t %1").arg(txt2);
        break;
    case QtInfoMsg:
        txt += QString("{Info} \t\t %1").arg(txt2);
        break;
    case QtWarningMsg:
        txt += QString("{Warning} \t %1").arg(txt2);
        break;
    case QtCriticalMsg:
        txt += QString("{Critical} \t %1").arg(txt2);
        break;
    case QtFatalMsg:
        txt += QString("{Fatal} \t\t %1").arg(txt2);
        abort();
        break;
    }

    QFile outFile("LogFile.log");
    outFile.open(QIODevice::WriteOnly | QIODevice::Append);

    QTextStream textStream(&outFile);
    textStream << txt << Qt::endl;
}

void Run::listAvailableAddresses()
{
    availableAddresses.clear();
    const QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
    availableAddresses.reserve(ipAddressesList.size());

    for (const QHostAddress &ip : ipAddressesList) {
        if (true) {
            availableAddresses.push_back(ip);
        }
    }
    qout << Ecma48(64,255,64);
    qPrint(qout, tr("List of ip addresses:"));
    qout << EcmaSetter::AllDefault;
    qls(availableAddresses);
}

template<class T>
void Run::qls(const QList<T> input)
{
    if(input.length() <= 0)
    {
        return;
    }
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

    auto lengthcmp = [](T a, auto&& b) { return callength(a) < callength(b); };

    int maxcolumns = input.length();
    int total = input.length();
    QMap<int, QList<QList<T>>> displayCandidates;
    for(int columns = 1; columns <= maxcolumns; ++columns)
    {
        int rows;
        if(total % columns == 0)
        {
            rows = total / columns;
        }
        else
        {
            rows = total / columns + 1;
        }
        QList<QList<T>> displays;
        QList<T> singlecolumn;
        typedef typename QList<T>::const_iterator iter;
        for(iter i = input.constBegin(); i != input.constEnd(); ++i)
        {
            if(singlecolumn.length() < rows && i != (input.constEnd() - 1))
            {
                singlecolumn.append(*i);
            }
            else if (singlecolumn.length() < rows && i == (input.constEnd() - 1))
            {
                singlecolumn.append(*i);
                displays.append(singlecolumn);
            }
            else
            {
                displays.append(singlecolumn);
                singlecolumn = *(new QList<T>());
                --i;

            }
        }
        int displayedcolumns = 0;
        for(int j = 0; j < displays.length(); ++j)
        {
            displayedcolumns += (callength(*std::max_element(
                                               (displays.constBegin() + j)->constBegin(),
                                               (displays.constBegin() + j)->constEnd(),
                                               lengthcmp)) + 1);
        }
        displayedcolumns--;

        if(displayedcolumns <= width)
        {
            displayCandidates[rows] = displays;

        }
    }
    QList<int> k = displayCandidates.keys();
    int min = *std::min_element(k.begin(), k.end());
    QList<QList<T>> displaySelected = displayCandidates[min];

    for(int i = 0; i < displaySelected.begin()->length(); ++i)
    {
        for(int j = 0; j < displaySelected.length(); ++j)
        {
            if(i < (displaySelected.begin() + j)->length())
            {
                T current_element = *((displaySelected.begin() + j)->constBegin() + i);
                int fieldwidth = callength(*std::max_element(
                                               (displaySelected.begin() + j)->begin(),
                                               (displaySelected.begin() + j)->end(),
                                               lengthcmp)) + ((j == displaySelected.length() - 1) ? 0 : 1)
                        - callength(current_element)
                        + callength(current_element, true);
                qout.setFieldWidth(fieldwidth);
                QString str = strfiy(*((displaySelected.begin() + j)->constBegin() + i));
                qout << str;
            }
        }
        qout.setFieldWidth(0);
        qout << Qt::endl;
    }
    qout << Qt::endl;
}

inline int Run::callength(const QString &input, bool naive)
{
    if(naive)
    {
        return input.length();
    }
    else
    {
        const QChar *data = input.constData();
        return mk_wcswidth(data, input.size());
    }
}

inline int Run::callength(const QHostAddress &input, bool naive)
{
    Q_UNUSED(naive)
    return input.toString().length();
}

inline const QString & Run::strfiy(const QString &in)
{
    return in;
}

inline QString Run::strfiy(const QHostAddress &in)
{
    QString result = in.toString();
    return result;
}
