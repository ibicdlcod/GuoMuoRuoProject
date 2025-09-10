#ifndef INTERACTIVELABEL_H
#define INTERACTIVELABEL_H

#include <QLabel>
#include <QObject>
#include "equipview.h"

class InteractiveLabel : public QLabel
{
    Q_OBJECT
public:
    explicit InteractiveLabel(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent *event) override;

    EquipView *view;
    bool mousePressedInside = false;
};

#endif // INTERACTIVELABEL_H
