#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "clientv2.h"
#include "factoryslot.h"
#include "portarea.h"
#include "licensearea.h"
#include "loginscreen.h"

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

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void adjustLicenseArea();
    void developClicked(bool checked = false, int slotnum = 0);
    void doFactoryRefresh(const QJsonObject &);
    void gamestateChanged(KP::GameState);
    void parseConnectReq();
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

    PortArea *portArea;
    LicenseArea *licenseArea;
    LoginScreen *loginScreen;
};
#endif // MAINWINDOW_H
