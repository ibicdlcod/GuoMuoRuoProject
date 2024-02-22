#ifndef SERVERMASTERSENDER_H
#define SERVERMASTERSENDER_H

#include <QObject>
#include <QAbstractSocket>
#include "../Protocol/sender.h"

class ServerMasterSender : public QObject
{
    Q_OBJECT
public:
    explicit ServerMasterSender(QObject *parent = nullptr);

signals:

public slots:
    void addSender(QAbstractSocket *);
    void removeSender(QAbstractSocket *);
    void sendMessage(QAbstractSocket *, const QByteArray &);
    int numberofMembers() const;

private:
    QMap<QAbstractSocket *, Sender *> agents;
};

#endif // SERVERMASTERSENDER_H
