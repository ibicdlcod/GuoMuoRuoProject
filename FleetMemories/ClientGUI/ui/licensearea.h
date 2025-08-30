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

signals:
    void showLicenseComplete();

public slots:
    void complete();
    void neverComplete();

private:
    Ui::LicenseArea *ui;
    bool completeOff = false;
};

#endif // LICENSEAREA_H
