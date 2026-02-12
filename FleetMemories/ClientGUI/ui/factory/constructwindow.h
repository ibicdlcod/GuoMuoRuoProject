#ifndef CONSTRUCTWINDOW_H
#define CONSTRUCTWINDOW_H

#include <QDialog>
#include "../../model/shipdefmodel.h"

namespace Ui {
class ConstructWindow;
}

class ConstructWindow : public QDialog
{
    Q_OBJECT

public:
    explicit ConstructWindow(QWidget *parent = nullptr);
    ~ConstructWindow();

    void initialize();

public slots:
    void switchDisplay(int dummy = 0);
    void shipNameChanged(int dummy = 0);

private:
    Ui::ConstructWindow *ui;
};

#endif // CONSTRUCTWINDOW_H
