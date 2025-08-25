#ifndef MAPRENDER_H
#define MAPRENDER_H

#include <QWidget>
#include <QPen>

class MapRender : public QWidget
{
    Q_OBJECT
public:
    explicit MapRender(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QPen pen;
    QBrush brush;
    bool antialiased;
    QPixmap pixmap;
};

#endif // MAPRENDER_H
