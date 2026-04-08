/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedLayout>
#include "../../FactorySlot/factoryslot.h"
#include "port/portarea.h"
#include "port/licensearea.h"
#include "port/newlogins.h"
#include "factory/factoryarea.h"
#include "techview.h"
#include "sortie/sortie.h"
#include "fleet/fleetview.h"
#include "maintenance/repair.h"
#include "shop/ardcoupondialog.h"
#include "shop/buyequipdialog.h"
#include "shop/buyordresourcesdialog.h"
#include "shop/medalbuydialog.h"
#include "settingswindow.h"

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

    FleetView * getFleetArea() const;
    QLayout * getFleetAreaWidget() const;
    friend class ConfirmSortie;

public slots:
    void lockBattle();
    void unlockBattle();

signals:
    void cmdMessage(const QString &);

protected:
    void resizeEvent(QResizeEvent *) override;
    void showEvent(QShowEvent *event) override;

private slots:
    void adjust(int dummy = 0);
    void adjustArea(QWidget *, const QSize &);
    void buyARD();
    void buyEquip();
    void buyMedal();
    void buyOrdResources();
    void factoryRefresh();
    void gamestateChanged(KP::GameState);
    void gamestateInit();
    void printMessage(QString, QColor background = QColor("white"),
                      QColor foreground = QColor("black"));
    void processCmd();
    void switchToAnchorage();
    void switchToArsenal();
    void switchToBlueprint();
    void switchToCloningVats();
    void switchToConstruct();
    void switchToDevelop();
    void showResourceGain();
    void switchToFleet();
    void switchToRank();
    void switchToSortie();
    void updateColorScheme(Qt::ColorScheme colorscheme);
    void updateResources(const QJsonObject &);

private:
    Ui::MainWindow *ui;
    bool pwConfirmMode = false;
    bool m_firstShow = true;
    QList<FactorySlot *> slotfs;

    FactoryArea *factoryArea;
    LicenseArea *licenseArea;
    NewLoginS *newLoginScreen;
    PortArea *portArea;
    TechView *techArea;
    Sortie *battleArea;
    FleetView *fleetArea;
    Repair *repairArea;
    SettingsWindow settingsWindow;

    QStackedLayout *lay;

    QList<QMetaObject::Connection> v; // volatile connections
};
#endif // MAINWINDOW_H
