#include "industrialselect.h"
#include <QHBoxLayout>

IndustrialSelect::IndustrialSelect(int height, QWidget *parent)
    : height(height), QWidget{parent}
{
    iPLabel = new QLabel(this);
    //% "Your Industrial Points:"
    iPLabel->setText(qtTrId("ipLabel"));
    iPValueLabel = new QLabel(this);
    //% "Calculating..."
    iPValueLabel->setText(qtTrId("ip-calculating"));
    buyButton = new QPushButton(this);
    //% "Request Equip"
    buyButton->setText(qtTrId("buy-equip-button"));

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(iPLabel);
    layout->addWidget(iPValueLabel);
    layout->addWidget(buyButton);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(6);
    layout->setAlignment(Qt::AlignCenter);

    QSizePolicy labelSize = QSizePolicy(QSizePolicy::Maximum,
                                        QSizePolicy::Preferred,
                                        QSizePolicy::Label);
    iPLabel->setSizePolicy(labelSize);
    iPValueLabel->setSizePolicy(labelSize);
    iPLabel->setMinimumSize(QSize(100, height));
    iPValueLabel->setMinimumSize(QSize(100, height));

    buyButton->setSizePolicy(QSizePolicy(QSizePolicy::Maximum,
                                             QSizePolicy::Preferred,
                                             QSizePolicy::PushButton));
    buyButton->resize(QSize(100, height));

    connect(buyButton, &QAbstractButton::clicked,
            this, &IndustrialSelect::buyActivated);
}

void IndustrialSelect::setIPValue(double value) {
    iPValueLabel->setText(QString::number(value, 'g', 6));
}
