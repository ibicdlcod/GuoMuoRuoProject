#ifndef FLEETVIEW_H
#define FLEETVIEW_H

#include <QFrame>

namespace Ui {
class FleetView;
}

class FleetView : public QFrame
{
    Q_OBJECT

public:
    explicit FleetView(QWidget *parent = nullptr);
    ~FleetView();

private:
    Ui::FleetView *ui;
};

#endif // FLEETVIEW_H
