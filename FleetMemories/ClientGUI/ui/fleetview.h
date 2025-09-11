#ifndef FLEETVIEW_H
#define FLEETVIEW_H

#include <QFrame>
#include "equipview.h"

namespace Ui {
class FleetView;
}

class FleetView : public QFrame
{
    Q_OBJECT

public:
    explicit FleetView(QWidget *parent = nullptr);
    ~FleetView();
    EquipView equipView;

private:
    Ui::FleetView *ui;
};

#endif // FLEETVIEW_H
