#ifndef STENGINE_H
#define STENGINE_H

#include <QObject>

#include "strelations.h"

class STEngine : public QObject
{
    Q_OBJECT
public:
    explicit STEngine(QObject *parent = nullptr);

signals:
    void stateChanged(); // TBD: GUI

    void invalidCommand();
    void showAllCommands();
    void showCommands(const QList<STCType>);
    void showHelp(QStringList);

public slots:
    bool parse(QString command);
    QString getState();

private:
    STState state;
};

#endif // STENGINE_H
