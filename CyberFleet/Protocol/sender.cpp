#include "sender.h"
#include <QDebug>
#include <QTimer>
#include <QSettings>
#include "kp.h"

extern std::unique_ptr<QSettings> settings;
/* this is deliberately not customized */
#pragma message(NOT_M_CONST)
namespace {
const int practicalBufferSize = 1024;
}

Sender::Sender(QAbstractSocket *destination,
               QObject *parent) :
    QObject(parent),
    m_destination(destination),
    m_buffer(practicalBufferSize, Qt::Uninitialized),
    m_hasRead(0),
    m_hasWritten(0),
    m_doneSignaled(false),
    m_readySend(true),
    m_partnum(0),
    m_partnumtotal(0)
{
    Q_ASSERT(m_destination->isWritable());
    // see bool Connection::operator bool()
    /*
    connect(m_destination, &QAbstractSocket::bytesWritten,
                     this, &Sender::destinationBytesWritten);
    connect(m_destination, &QAbstractSocket::errorOccurred,
                     this, &Sender::destinationError);*/
}

void Sender::enqueue(const QByteArray &content) {
    input.enqueue(content);
    start();
}

void Sender::start() {
    if(input.isEmpty()) {
        //% "Input buffer is empty."
        qWarning() << qtTrId("input-buffer-empty");
        return;
    }
    if(m_readySend) {
        connect(m_destination, &QAbstractSocket::bytesWritten,
                this, &Sender::destinationBytesWritten);
        connect(m_destination, &QAbstractSocket::errorOccurred,
                this, &Sender::destinationError);
        buffer.setBuffer(&(input.first()));
        buffer.open(QBuffer::ReadOnly);
        m_source = &buffer;

        m_partnum = 0;
        m_partnumtotal = 0;
        m_doneSignaled = false;
        m_hasRead = 0;
        m_hasWritten = 0;
        m_sourceSize = m_source->size();
        send();
    }
    else {

    }
}

void Sender::destinationBytesWritten(qint64 length) {
    if (m_destination->bytesToWrite() < m_buffer.size() / 2) {
        // the transmit buffer is running low, refill]
        send();
    }
    m_hasWritten += length;
    emit progressed((m_hasWritten * 100) / m_sourceSize);
    signalDone();
}

void Sender::destinationError() {
    //% "Unable to send data."
    emit errorOccurred(qtTrId("unable-send-data"));
    qWarning() << m_destination->error();
}

void Sender::send() {
    m_readySend = false;
    if (signalDone()) {
        disconnect(m_destination, &QAbstractSocket::bytesWritten,
                   this, &Sender::destinationBytesWritten);
        disconnect(m_destination, &QAbstractSocket::errorOccurred,
                   this, &Sender::destinationError);
        input.dequeue();
        buffer.close();

        m_partnum = 0;
        m_partnumtotal = 0;
        QTimer::singleShot(settings->value(
                                       "networkshared/mintimebetweenmsgsinms",
                                       100).toInt(),
                           this, &Sender::switchtoReady);
        return;
    }
    if(m_partnum == 0 && m_source->bytesAvailable()) {
        m_partnumtotal = (m_source->bytesAvailable() - 1)
                             / practicalBufferSize + 1;
        messageId = QUuid::createUuid();
    }
    qint64 read = m_source->read(m_buffer.data(), m_buffer.size());
    if (read == -1) {
        //% "Error reading intended message."
        emit errorOccurred(qtTrId("error-reading-intended-message"));
        return;
    }
    m_hasRead += read;
    QString prependerString = QStringLiteral("<start>")
                              + QString::number(m_partnumtotal)
                              + QStringLiteral("<mid>")
                              + QString::number(m_partnum)
                              + QStringLiteral("<mid>")
                              + messageId.toString(QUuid::WithoutBraces)
                              + QStringLiteral("<mid>");
    QByteArray prepender = prependerString.toLatin1();
    qint64 preSize = prepender.size();
    QByteArray appender("<end>");
    prepender += m_buffer;
    /* prepender have uninitialized trailing bits */
    QByteArray writeContent = prepender.first(read + preSize) + appender;
    qint64 writeSize = writeContent.size();
    qint64 written = m_destination->write(writeContent.constData(),
                                          writeSize);
    if (written == -1) {
        //% "Unable to send data, connection broke."
        emit errorOccurred(qtTrId("unable-send-data-connection-broke"));
        qWarning() << m_destination->error();
        return;
    }
    if (written != writeSize) {
        //% "Write buffer failed to fill completely."
        emit errorOccurred(qtTrId("write-buffer-failed-to-fill"));
        qWarning() << m_destination->error();
        return;
    }
    m_partnum++;
}

bool Sender::signalDone() {
    if (!m_doneSignaled &&
        ((m_sourceSize && m_hasWritten == m_sourceSize) || m_source->atEnd())) {
        emit done();
        m_doneSignaled = true;
    }
    return m_doneSignaled;
}

void Sender::switchtoReady() {
    m_readySend = true;
    if(!input.isEmpty()) {
        start();
    }
}

QHostAddress Sender::peerAddress() {
    return m_destination->peerAddress();
}

quint16 Sender::peerPort() {
    return m_destination->peerPort();
}
