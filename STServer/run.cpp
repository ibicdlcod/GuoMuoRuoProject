#include "run.h"

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
        qout << Ecma48(192,255,192,true) << Ecma48(0,0,0);
        qout << qSetFieldWidth(width)
             << "What? Admiral Tanaka? He's the real deal, isn't he? Great at battle and bad at politics--so cool!"
             << qSetFieldWidth(0) << Qt::endl;
    }
    qout << EcmaSetter::AllDefault;
    qout.setFieldAlignment(QTextStream::AlignLeft);
    qout << Qt::endl;

    listAvailableAddresses();

    emit finished();
    return;
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
    qPrint(qout, "List of ip addresses:");
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
    qout << EcmaSetter::AllDefault;

    for(int i = 0; i < displaySelected.begin()->length(); ++i)
    {
        for(int j = 0; j < displaySelected.length(); ++j)
        {
            T current_element = *((displaySelected.begin() + j)->constBegin() + i);
            int fieldwidth = callength(*std::max_element(
                                           (displaySelected.begin() + j)->begin(),
                                           (displaySelected.begin() + j)->end(),
                                           lengthcmp)) + ((j == displaySelected.length() - 1) ? 0 : 1)
                    - callength(current_element)
                    + callength(current_element, true);
            if(i < (displaySelected.begin() + j)->length())
            {
                qout << qSetFieldWidth(fieldwidth);
                QString str = strfiy(*((displaySelected.begin() + j)->constBegin() + i));
                qout << str;
            }
        }
        qout << qSetFieldWidth(0) << Qt::endl;
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
