#include "sender.h"
#include <QDebug>
#include <QTimer>

/* TODO: this should be customizeable */
static int practicalBufferSize = 1024;

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
    Q_ASSERT(connect(m_destination, &QAbstractSocket::bytesWritten,
                     this, &Sender::destinationBytesWritten));
    Q_ASSERT(connect(m_destination, &QAbstractSocket::errorOccurred,
                     this, &Sender::destinationError));
}

void Sender::enque(const QByteArray &content) {
    qWarning() << "enque";
    input.enqueue(content);
    start();
}

void Sender::start() {
    if(input.isEmpty()) {
        qWarning() << qtTrId("input-buffer-empty");
        return;
    }
    if(m_readySend) {
        buffer.setBuffer(&(input.first()));
        buffer.open(QBuffer::ReadOnly);
        m_source = &buffer;

        qWarning() << "ready";
        m_partnum = 0;
        m_partnumtotal = 0;
        m_doneSignaled = false;
        m_hasRead = 0;
        m_hasWritten = 0;
        m_sourceSize = m_source->size();
        send();
    }
    else {
        qWarning() << "notready";
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
    emit errorOccurred(qtTrId("unable-send-data"));
    qWarning() << m_destination->error();
}

void Sender::send() {
    m_readySend = false;
    if (signalDone()) {
        qWarning() << "deque";
        input.dequeue();
        buffer.close();

        m_partnum = 0;
        m_partnumtotal = 0;
        qWarning() << "switchready1000";
        QTimer::singleShot(1000, this, &Sender::switchtoReady);
        return;
    }
    if(m_partnum == 0 && m_source->bytesAvailable()) {
        m_partnumtotal = (m_source->bytesAvailable() - 1)
                             / practicalBufferSize + 1;
        messageId = QUuid::createUuid();
    }
    qint64 read = m_source->read(m_buffer.data(), m_buffer.size());
    if (read == -1) {
        emit errorOccurred(qtTrId("error-reading-intended-message"));
        return;
    }
    m_hasRead += read;
    QString prependerString = QStringLiteral("\001")
                              + QString::number(m_partnumtotal)
                              + QStringLiteral("\002")
                              + QString::number(m_partnum)
                              + QStringLiteral("\002")
                              + messageId.toString(QUuid::WithoutBraces)
                              + QStringLiteral("\003");
    QByteArray prepender = prependerString.toLatin1();
    qint64 preSize = prepender.size();
    prepender += m_buffer;
    qint64 written = m_destination->write(prepender.constData(),
                                          read + preSize);
    if (written == -1) {
        emit errorOccurred(qtTrId("unable-send-data-connection-broke"));
        qWarning() << m_destination->error();
        return;
    }
    if (written != read + preSize) {
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
    qWarning() << "switchready";
    m_readySend = true;
    if(!input.isEmpty()) {
        start();
    }
}

