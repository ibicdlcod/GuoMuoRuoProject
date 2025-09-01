#ifndef SHIPSELECT_H
#define SHIPSELECT_H

#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>

class ShipSelect : public QWidget
{
    Q_OBJECT
public:
    explicit ShipSelect(int height, QWidget *parent = nullptr);

signals:
    void selectChanged(const QString &nationality,
                       const QString &shiptype,
                       const QString &shipclass,
                       const QString &searchTerm
                       = QLatin1String(""));

public slots:
    void typeBoxHinted(const QStringList &types);
    void classBoxHinted(const QStringList &types);

public:
    QLabel *searchLabel;
    QLineEdit *searchBox;
    QLabel *nationLabel;
    QComboBox *nationBox;
    QLabel *typeLabel;
    QComboBox *typeBox;
    QLabel *classLabel;
    QComboBox *classBox;

    QPushButton *addStarButton;

private:
    int height;
};

#endif // SHIPSELECT_H
