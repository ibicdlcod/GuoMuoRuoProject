#ifndef STCOMMAND_H
#define STCOMMAND_H

#include <QObject>

#include "strelations.h"

class STCommand : public QObject
{
    Q_OBJECT
public:
    explicit STCommand(QObject *parent = nullptr, STCType type = STCType::help);

signals:

public slots:
    //virtual void execute();

protected:
    STCType type;
};

#endif // STCOMMAND_H
