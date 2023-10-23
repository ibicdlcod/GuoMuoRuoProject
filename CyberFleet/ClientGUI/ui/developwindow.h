#ifndef DEVELOPWINDOW_H
#define DEVELOPWINDOW_H

#include <QObject>
#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui { class DevelopWindow; }
QT_END_NAMESPACE

class DevelopWindow : public QDialog
{
    Q_OBJECT
public:
    explicit DevelopWindow(QWidget *parent = nullptr);
    ~DevelopWindow();

    int equipIdDesired();

signals:

public slots:
    void resetListName(int);
    void resetEquipName(int);

private:
    Ui::DevelopWindow *ui;
    bool initial = true;
};

#endif // DEVELOPWINDOW_H
