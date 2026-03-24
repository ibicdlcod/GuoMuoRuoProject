#ifndef RANKVIEW_H
#define RANKVIEW_H

#include <QWidget>

namespace Ui {
class RankView;
}

class RankView : public QWidget
{
    Q_OBJECT

public:
    explicit RankView(QWidget *parent = nullptr);
    ~RankView();

private:
    Ui::RankView *ui;
};

#endif // RANKVIEW_H
