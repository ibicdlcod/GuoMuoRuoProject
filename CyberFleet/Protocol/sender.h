#ifndef SENDER_H
#define SENDER_H

/* https://stackoverflow.com/questions/18861298/
 * when-to-check-for-error-when-using-qiodevices-
 * blocking-interface-qtcpsocket-an/20864012
 */

#include <QTcpSocket>
#include <QByteArray>
#include <QUuid>
#include <QQueue>
#include <QBuffer>

class Sender : public QObject
{
    Q_OBJECT
public:
    explicit Sender(QIODevice *,
                    QAbstractSocket *,
                    QObject *parent = nullptr);

signals:
    void done();
    void errorOccurred(const QString &);
    void progressed(int);

public slots:
    void enque(const QByteArray &);
    void start();

private slots:
    void destinationBytesWritten(qint64);
    void destinationError();
    void switchtoReady();

private:
    void send();
    bool signalDone();

    QQueue<QByteArray> input;
    QBuffer buffer;
    QIODevice *m_source;
    QAbstractSocket *m_destination;
    QByteArray m_buffer;
    qint64 m_hasRead;
    qint64 m_hasWritten;
    qint64 m_sourceSize;
    bool m_doneSignaled;
    bool m_readySend;
    bool m_pendingStart;
    qint64 m_partnum;
    qint64 m_partnumtotal;
    QUuid messageId;
};

#endif // SENDER_H
