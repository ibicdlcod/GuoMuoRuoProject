#ifndef CONSTRUCTWINDOW_H
#define CONSTRUCTWINDOW_H

#include <QDialog>

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
    void switchDisplay(int);

private:
    Ui::ConstructWindow *ui;
};

#endif // CONSTRUCTWINDOW_H
