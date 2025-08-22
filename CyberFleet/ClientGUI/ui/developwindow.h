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
    void displaySuccessRate2();

signals:
    void devDemandGlobalTech();
    void devDemandLocalTech(int);

public slots:
    void resetListName(int);
    void resetEquipName(int);

private slots:
    void displaySuccessRate(int);

private:
    Ui::DevelopWindow *ui;
    bool initial = true;
};

#endif // DEVELOPWINDOW_H
