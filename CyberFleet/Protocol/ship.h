#ifndef SHIP_H
#define SHIP_H

#include <QObject>

class Ship : public QObject
{
    Q_OBJECT
public:
    explicit Ship(QObject *parent = nullptr);

signals:
};

#endif // SHIP_H
