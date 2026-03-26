#ifndef INDUSTRIALSELECT_H
#define INDUSTRIALSELECT_H

#include <QObject>
#include <QLabel>
#include <QPushButton>

class IndustrialSelect : public QWidget
{
    Q_OBJECT
public:
    explicit IndustrialSelect(int height, QWidget *parent = nullptr);

signals:
    void buyActivated(bool checked = false);

public slots:
    void setIPValue(double);

public:
    QLabel *iPLabel; // industrial points
    QLabel *iPValueLabel;

    QPushButton *buyButton;

private:
    int height;
};

#endif // INDUSTRIALSELECT_H
