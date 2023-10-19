#ifndef TECHVIEW_H
#define TECHVIEW_H

#include <QFrame>

namespace Ui {
class TechView;
}

class TechView : public QFrame
{
    Q_OBJECT

public:
    explicit TechView(QWidget *parent = nullptr);
    ~TechView();

private:
    Ui::TechView *ui;
};

#endif // TECHVIEW_H
