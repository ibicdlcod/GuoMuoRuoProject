#ifndef QSSLSERVER_H
#define QSSLSERVER_H

#include <QTcpServer>
#include <QSslCertificate>

class QSslServer : public QTcpServer {
    Q_OBJECT
public:
    explicit QSslServer(QObject *parent = nullptr);
    virtual ~QSslServer() = default;

    virtual void incomingConnection(qintptr socketDescriptor) override;

public slots:
    void ready();

private:
    QList<QSslCertificate> certs;
};

#endif // QSSLSERVER_H
