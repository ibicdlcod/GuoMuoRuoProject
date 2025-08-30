#ifndef KEYENTERRECEIVER_H
#define KEYENTERRECEIVER_H

#include <QObject>

class KeyEnterReceiver : public QObject
{
    Q_OBJECT
public:
    explicit KeyEnterReceiver(QObject *parent = nullptr);

signals:
    void enterPressed();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
};

#endif // KEYENTERRECEIVER_H
