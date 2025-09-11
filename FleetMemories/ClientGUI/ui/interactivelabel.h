#ifndef INTERACTIVELABEL_H
#define INTERACTIVELABEL_H

#include <QLabel>
#include <QObject>
#include "fleetview.h"

class InteractiveLabel : public QLabel
{
    Q_OBJECT
public:
    explicit InteractiveLabel(FleetView* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    QUuid shipUID;

public slots:
    void shipSelected(QUuid);

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent *event) override;

    bool mousePressedInside = false;

    FleetView * parentView;
};

#endif // INTERACTIVELABEL_H
