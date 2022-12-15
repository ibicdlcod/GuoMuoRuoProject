#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "clientv2.h"

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
    void receivedMessage(const QString &);

private slots:
    void gamestateChanged(KP::GameState);
    void parseConnectReq();
    void parseRegReq();
    void printMessage(QString, QColor background = QColor("white"),
                      QColor foreground = QColor("black"));
    void processCmd();
    void switchToDevelop();

private:
    Ui::MainWindow *ui;
    bool pwConfirmMode = false;
    KP::FactoryState factoryState = KP::Development;
};
#endif // MAINWINDOW_H
