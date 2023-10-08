#ifndef LICENSEAREA_H
#define LICENSEAREA_H

#include <QFrame>

namespace Ui {
class LicenseArea;
}

class LicenseArea : public QFrame
{
    Q_OBJECT

public:
    explicit LicenseArea(QWidget *parent = nullptr);
    ~LicenseArea();

private:
    Ui::LicenseArea *ui;
};

#endif // LICENSEAREA_H
