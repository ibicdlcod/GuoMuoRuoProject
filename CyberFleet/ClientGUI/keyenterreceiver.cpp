#include "keyenterreceiver.h"
#include <QEvent>
#include <QKeyEvent>

KeyEnterReceiver::KeyEnterReceiver(QObject *parent)
    : QObject{parent}
{

}

bool KeyEnterReceiver::eventFilter(QObject *obj, QEvent *event) {
    if(event->type() == QEvent::KeyPress) {
        QKeyEvent *key = static_cast<QKeyEvent *>(event);
        if((key->key() == Qt::Key_Enter) || (key->key() == Qt::Key_Return)) {
            emit enterPressed();
        } else {
            return QObject::eventFilter(obj, event);
        }
        return true;
    } else {
        return QObject::eventFilter(obj, event);
    }
    return false;
}
