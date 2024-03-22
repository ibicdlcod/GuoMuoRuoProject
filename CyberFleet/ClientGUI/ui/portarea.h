#ifndef PORTAREA_H
#define PORTAREA_H

#include <QFrame>

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

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    Ui::PortArea *ui;
};

#endif // PORTAREA_H
