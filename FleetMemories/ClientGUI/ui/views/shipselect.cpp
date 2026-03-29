/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "shipselect.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QMetaEnum>

#include "../../../Protocol/kp.h"
#include "../../model/shipbpmodel.h"
#include "../../model/shipmodel.h"
#include "../factory/factoryarea.h"
#include "../fleet/fleetview.h"

ShipSelect::ShipSelect(int height, QWidget *parent)
    : height(height), QWidget{parent}
{
    searchLabel = new QLabel(this);
    //% "Search:"
    searchLabel->setText(qtTrId("equipview-search"));
    searchBox = new QLineEdit(this);
    searchBox->setObjectName("searchbox");
    searchBox->setMinimumSize(QSize(100, height));
    nationLabel = new QLabel(this);
    //% "Nationality:"
    nationLabel->setText(qtTrId("shipview-nation"));
    nationBox = new QComboBox(this);
    typeLabel = new QLabel(this);
    //% "Type:"
    typeLabel->setText(qtTrId("shipview-type"));
    typeBox = new QComboBox(this);
    classLabel = new QLabel(this);
    //% "Class:"
    classLabel->setText(qtTrId("shipview-class"));
    classBox = new QComboBox(this);

    addStarButton = new QPushButton(this);
    decorateButton = new QPushButton(this);
    supplyAllButton = new QPushButton(this);
    supplyAmmoButton = new QPushButton(this);
    supplyFuelButton = new QPushButton(this);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(searchLabel);
    layout->addWidget(searchBox);
    layout->addWidget(nationLabel);
    layout->addWidget(nationBox);
    layout->addWidget(typeLabel);
    layout->addWidget(typeBox);
    layout->addWidget(classLabel);
    layout->addWidget(classBox);
    layout->addWidget(addStarButton);
    layout->addWidget(decorateButton);
    layout->addWidget(supplyFuelButton);
    layout->addWidget(supplyAmmoButton);
    layout->addWidget(supplyAllButton);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(6);
    layout->setAlignment(Qt::AlignCenter);

    //% "Modernize"
    addStarButton->setText(qtTrId("add-star-button-ship"));
    //% "Decorate"
    decorateButton->setText(qtTrId("decorate-button-ship"));
    //% "Supply Fuel"
    supplyFuelButton->setText(qtTrId("supply-fuel-button"));
    //% "Supply Ammo"
    supplyAmmoButton->setText(qtTrId("supply-ammo-button"));
    //% "Supply All"
    supplyAllButton->setText(qtTrId("supply-all-button"));

    QSizePolicy labelSize = QSizePolicy(QSizePolicy::Maximum,
                                        QSizePolicy::Preferred,
                                        QSizePolicy::Label);
    searchLabel->setSizePolicy(labelSize);
    QSizePolicy textEditSize = QSizePolicy(QSizePolicy::Maximum,
                                           QSizePolicy::Maximum,
                                           QSizePolicy::Label);
    searchBox->setSizePolicy(textEditSize);
    searchBox->setStyleSheet(QStringLiteral(
        "QLineEdit#searchbox { background-color: palette(button); }"
        ));
    //searchBox->setSizeAdjustPolicy(QLineEdit::AdjustToContents);
    //searchBox->setMaximumSize(QSize(50, 10));
    searchBox->setMinimumSize(QSize(150, height));

    nationLabel->setSizePolicy(labelSize);
    nationBox->setSizePolicy(QSizePolicy(QSizePolicy::Maximum,
                                         QSizePolicy::Preferred,
                                         QSizePolicy::ComboBox));
    nationBox->resize(QSize(100, height));
    nationBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    auto meta = QMetaEnum::fromType<KP::AllegianceGroup>();
    for(int i = 0; i < meta.keyCount(); ++i) {
        if(meta.value(i) == KP::AllegianceGroup::UnknownNation) {
            //% "All nationalities"
            nationBox->addItem(qtTrId("all-nationality"));
        }
        else {
            nationBox->addItem(qtTrId(meta.key(i)));
        }
    }

    typeLabel->setSizePolicy(labelSize);
    typeBox->setSizePolicy(QSizePolicy(QSizePolicy::Maximum,
                                       QSizePolicy::Preferred,
                                       QSizePolicy::ComboBox));
    typeBox->resize(QSize(100, height));
    typeBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);

    classLabel->setSizePolicy(labelSize);
    classBox->setSizePolicy(QSizePolicy(QSizePolicy::Maximum,
                                        QSizePolicy::Preferred,
                                        QSizePolicy::ComboBox));
    classBox->resize(QSize(100, height));
    classBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);

    addStarButton->setSizePolicy(QSizePolicy(QSizePolicy::Maximum,
                                             QSizePolicy::Preferred,
                                             QSizePolicy::PushButton));
    addStarButton->resize(QSize(100, height));
    decorateButton->setSizePolicy(QSizePolicy(QSizePolicy::Maximum,
                                              QSizePolicy::Preferred,
                                              QSizePolicy::PushButton));
    decorateButton->resize(QSize(100, height));
    supplyFuelButton->setSizePolicy(
        QSizePolicy(QSizePolicy::Maximum,
                    QSizePolicy::Preferred,
                    QSizePolicy::PushButton));
    supplyFuelButton->resize(QSize(100, height));
    supplyAmmoButton->setSizePolicy(
        QSizePolicy(QSizePolicy::Maximum,
                    QSizePolicy::Preferred,
                    QSizePolicy::PushButton));
    supplyAmmoButton->resize(QSize(100, height));
    supplyAllButton->setSizePolicy(
        QSizePolicy(QSizePolicy::Maximum,
                    QSizePolicy::Preferred,
                    QSizePolicy::PushButton));
    supplyAllButton->resize(QSize(100, height));

    connect(addStarButton, &QAbstractButton::clicked,
            this, &ShipSelect::modernizeActivated);
    connect(decorateButton, &QAbstractButton::clicked,
            this, &ShipSelect::decorateActivated);
    connect(supplyFuelButton, &QAbstractButton::clicked,
            this, &ShipSelect::supplyFuelActivated);
    connect(supplyAmmoButton, &QAbstractButton::clicked,
            this, &ShipSelect::supplyAmmoActivated);
    connect(supplyAllButton, &QAbstractButton::clicked,
            this, &ShipSelect::supplyAllActivated);

    connect(nationBox, &QComboBox::activated,
            this, [this]{
                emit selectChanged(
                    nationBox->currentText().
                            localeAwareCompare(qtTrId("all-nationality")) == 0
                        ? QLatin1String("") : nationBox->currentText(),
                    QLatin1String(""),
                    QLatin1String(""),
                    QLatin1String(""));
            });
    connect(typeBox, &QComboBox::activated,
            this, [this]{
                emit selectChanged(
                    nationBox->currentText().
                            localeAwareCompare(qtTrId("all-nationality")) == 0
                        ? QLatin1String("") : nationBox->currentText(),
                    typeBox->currentText().
                            localeAwareCompare(qtTrId("all-shiptypes")) == 0
                        ? QLatin1String("") : typeBox->currentText(),
                    QLatin1String(""),
                    QLatin1String(""));
            });
    connect(classBox, &QComboBox::activated,
            this, [this]{
                emit selectChanged(
                    nationBox->currentText().
                            localeAwareCompare(qtTrId("all-nationality")) == 0
                        ? QLatin1String("") : nationBox->currentText(),
                    typeBox->currentText().
                            localeAwareCompare(qtTrId("all-shiptypes")) == 0
                        ? QLatin1String("") : typeBox->currentText(),
                    classBox->currentText().
                            localeAwareCompare(qtTrId("all-shipclasses")) == 0
                        ? QLatin1String("") : classBox->currentText(),
                    QLatin1String(""));
            });
    connect(searchBox, &QLineEdit::textEdited,
            this, [this]{
                emit selectChanged(QLatin1String(""),
                                   QLatin1String(""),
                                   QLatin1String(""),
                                   searchBox->text());
            });
}

void ShipSelect::typeBoxHinted(QStringList &types) {
    FleetView * fleetView = nullptr;
    for(auto *widget: QApplication::allWidgets()) {
        if(qobject_cast<FleetView *>(widget)) {
            fleetView = qobject_cast<FleetView *>(widget);
        }
    }
    FactoryArea * factoryArea = nullptr;
    for(auto *widget: QApplication::allWidgets()) {
        if(qobject_cast<FactoryArea *>(widget)) {
            factoryArea = qobject_cast<FactoryArea *>(widget);
        }
    }
    if(factoryArea == nullptr)
        return;
    if(factoryArea->getState() == KP::Anchorage
       && qobject_cast<ShipBPModel *>(QObject::sender())) {
        return;
    }
    if(fleetView && fleetView->isVisible()
       && qobject_cast<ShipBPModel *>(QObject::sender())) {
        return;
    }
    if(factoryArea->getState() == KP::BlueprintView
       && !qobject_cast<ShipBPModel *>(QObject::sender())) {
        return;
    }
    /* disconnect to eliminate infinite recursion */
    disconnect(qobject_cast<ShipModel *>(QObject::sender()),
               &ShipModel::typeBoxHint,
               this, &ShipSelect::typeBoxHinted);
    types.removeOne(qtTrId("all-shiptypes"));
    std::sort(types.begin(), types.end(), [](QString a, QString b)
              { return a.localeAwareCompare(b) < 0; });
    typeBox->clear();
    typeBox->addItem(qtTrId("all-shiptypes"));
    typeBox->addItems(types);
    classBox->clear();
    update();
    connect(qobject_cast<ShipModel *>(QObject::sender()),
            &ShipModel::typeBoxHint,
            this, &ShipSelect::typeBoxHinted);
}

void ShipSelect::classBoxHinted(QStringList &classes) {
    FleetView * fleetView = nullptr;
    for(auto *widget: QApplication::allWidgets()) {
        if(qobject_cast<FleetView *>(widget)) {
            fleetView = qobject_cast<FleetView *>(widget);
        }
    }
    FactoryArea * factoryArea = nullptr;
    for(auto *widget: QApplication::allWidgets()) {
        if(qobject_cast<FactoryArea *>(widget)) {
            factoryArea = qobject_cast<FactoryArea *>(widget);
        }
    }
    if(factoryArea == nullptr)
        return;
    if(factoryArea->getState() == KP::Anchorage
       && qobject_cast<ShipBPModel *>(QObject::sender())) {
        return;
    }
    if(fleetView && fleetView->isVisible()
       && qobject_cast<ShipBPModel *>(QObject::sender())) {
        return;
    }
    if(factoryArea->getState() == KP::BlueprintView
       && !qobject_cast<ShipBPModel *>(QObject::sender())) {
        return;
    }
    /* disconnect to eliminate infinite recursion */
    disconnect(qobject_cast<ShipModel *>(QObject::sender()),
               &ShipModel::classBoxHint,
               this, &ShipSelect::classBoxHinted);
    classes.removeOne(qtTrId("all-shipclasses"));
    std::sort(classes.begin(), classes.end(), [](QString a, QString b)
              { return a.localeAwareCompare(b) < 0; });
    classBox->clear();
    classBox->addItem(qtTrId("all-shipclasses"));
    classBox->addItems(classes);
    update();
    connect(qobject_cast<ShipModel *>(QObject::sender()),
            &ShipModel::classBoxHint,
            this, &ShipSelect::classBoxHinted);
}
