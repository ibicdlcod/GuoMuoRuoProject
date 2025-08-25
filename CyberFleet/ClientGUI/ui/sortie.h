#ifndef SORTIE_H
#define SORTIE_H

#include <QFrame>
#include <QLabel>
#include "../../Protocol/kp.h"
#include "maprender.h"
#include "mapviewwidget.h"

namespace Ui {
class Sortie;
}

class Sortie : public QFrame
{
    Q_OBJECT

public:
    explicit Sortie(QWidget *parent = nullptr);
    ~Sortie();

    void setState(KP::SortieState);
    void switchToState();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    Ui::Sortie *ui;
    MapRender *renderer;
    MapViewWidget *globeFrame;

    KP::SortieState sortieState = KP::MapView;
};

#endif // SORTIE_H
