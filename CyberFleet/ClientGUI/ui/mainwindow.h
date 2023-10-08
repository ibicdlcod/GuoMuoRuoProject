#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "clientv2.h"
#include "factoryslot.h"
#include "portarea.h"
#include "licensearea.h"
#include "newlogins.h"
#include "factoryarea.h"

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
    void adjustLoginArea();
    void developClicked(bool checked = false, int slotnum = 0);
    void doFactoryRefresh(const QJsonObject &);
    void gamestateChanged(KP::GameState);
    void gamestateInit();
    //void parseConnectReq();
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

    FactoryArea *factoryArea;
    LicenseArea *licenseArea;
    NewLoginS *newLoginScreen;
    PortArea *portArea;
};
#endif // MAINWINDOW_H
