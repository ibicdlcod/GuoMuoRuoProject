#ifndef CONFIRMSORTIE_H
#define CONFIRMSORTIE_H

#include <QDialog>
#include <QObject>
#include "ui_confirmsortie.h"
#include <QString>
#include "../fleet/fleetview.h"

class ConfirmSortie : public QDialog
{
    Q_OBJECT

public:
    explicit ConfirmSortie(QWidget *parent = nullptr,
                           QString mapText = QStringLiteral(""),
                           QString diffText = QStringLiteral(""));
    ~ConfirmSortie();

private:
    Ui::ConfirmSortie *ui;
    FleetView *fv;
};

#endif // CONFIRMSORTIE_H
