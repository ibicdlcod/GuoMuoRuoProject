#include "licensearea.h"
#include "ui_licensearea.h"

LicenseArea::LicenseArea(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::LicenseArea)
{
    ui->setupUi(this);
}

LicenseArea::~LicenseArea()
{
    delete ui;
}
