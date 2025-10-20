#ifndef PORTAREA_H
#define PORTAREA_H

#include <QFrame>
#include "choosehomeport.h"

namespace Ui {
class PortArea;
}

class PortArea : public QFrame
{
    Q_OBJECT

public:
    explicit PortArea(QWidget *parent = nullptr);
    ~PortArea();

public slots:
    void equipRegistryComplete();
    void hello();
    void mapRegistryComplete();
    void shipRegistryComplete();
    void showChooseHomePort(const QJsonObject &);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    Ui::PortArea *ui;
    ChooseHomePort *homeport = nullptr;
};

#endif // PORTAREA_H
