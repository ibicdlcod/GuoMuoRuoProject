#ifndef SERVERMASTERSENDER_H
#define SERVERMASTERSENDER_H

#include <QObject>
#include <QAbstractSocket>
#include "../Protocol/sender.h"

class serverMasterSender : public QObject
{
    Q_OBJECT
public:
    explicit serverMasterSender(QObject *parent = nullptr);

signals:

public slots:
    void addSender(QAbstractSocket *);
    void removeSender(QAbstractSocket *);
    void sendMessage(QAbstractSocket *, QByteArray);

private:
    QMap<QAbstractSocket *, Sender *> agents;
};

#endif // SERVERMASTERSENDER_H
