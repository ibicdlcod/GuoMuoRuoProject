#ifndef MAPDETAIL_H
#define MAPDETAIL_H

#include <QWidget>

namespace Ui {
class MapDetail;
}

class MapDetail : public QWidget
{
    Q_OBJECT

public:
    explicit MapDetail(QWidget *parent = nullptr);
    ~MapDetail();

private:
    Ui::MapDetail *ui;
};

#endif // MAPDETAIL_H
