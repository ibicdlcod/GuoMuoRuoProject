#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "../../FactorySlot/factoryslot.h"
#include "portarea.h"
#include "licensearea.h"
#include "newlogins.h"
#include "factoryarea.h"
#include "techview.h"
#include "sortie.h"

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
    void resizeEvent(QResizeEvent *) override;

private slots:
    void adjustArea(QFrame *, const QSize &);
    void factoryRefresh();
    void gamestateChanged(KP::GameState);
    void gamestateInit();
    void printMessage(QString, QColor background = QColor("white"),
                      QColor foreground = QColor("black"));
    void processCmd();
    void switchToArsenal();
    void switchToConstruct();
    void switchToDevelop();
    void switchToSortie();
    void updateColorScheme(Qt::ColorScheme colorscheme);
    void updateResources(const QJsonObject &);

private:
    Ui::MainWindow *ui;
    bool pwConfirmMode = false;
    QList<FactorySlot *> slotfs;

    FactoryArea *factoryArea;
    LicenseArea *licenseArea;
    NewLoginS *newLoginScreen;
    PortArea *portArea;
    TechView *techArea;
    Sortie *battleArea;
};
#endif // MAINWINDOW_H
