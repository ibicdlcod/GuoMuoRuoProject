#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "clientv2.h"
#include "factoryslot.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr,
               int argc = 0,
               char ** argv = nullptr);
    ~MainWindow();


signals:
    void cmdMessage(const QString &);

private slots:
    void developClicked(bool checked = false, int slotnum = 0);
    void doFactoryRefresh(const QJsonObject &);
    void gamestateChanged(KP::GameState);
    void parseConnectReq();
    void parseRegReq();
    void printMessage(QString, QColor background = QColor("white"),
                      QColor foreground = QColor("black"));
    void processCmd();
    void factoryRefresh();
    void switchToDevelop();

private:
    Ui::MainWindow *ui;
    bool pwConfirmMode = false;
    KP::FactoryState factoryState = KP::Development;
    QList<FactorySlot *> slotfs;
};
#endif // MAINWINDOW_H
