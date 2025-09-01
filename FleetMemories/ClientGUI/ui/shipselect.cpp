#include "shipselect.h"
#include <QHBoxLayout>
#include <QMetaEnum>
#include "../../Protocol/kp.h"

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
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(6);
    layout->setAlignment(Qt::AlignCenter);

    //% "Modernize"
    addStarButton->setText(qtTrId("add-star-button-ship"));


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
    auto meta = QMetaEnum::fromType<KP::ShipNationality>();
    for(int i = 0; i < meta.keyCount(); ++i) {
        if(meta.value(i) == KP::ShipNationality::Unknown) {
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

    /*
    connect(typeBox, &QComboBox::activated,
            this, &EquipSelect::typeActivated);
    connect(equipBox, &QComboBox::activated,
            this, [this]{
                emit equipActivated(equipBox->currentText());
            });
    connect(destructButton, &QAbstractButton::clicked,
            this, &EquipSelect::destructActivated);
    connect(searchBox, &QLineEdit::textEdited,
            this, &EquipSelect::searchBoxChanged);
    connect(this, &EquipSelect::typeActivated,
            this, &EquipSelect::reCalculateAvailableEquips);
*/
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

void ShipSelect::typeBoxHinted(const QStringList &types) {
    typeBox->clear();
    typeBox->addItems(types);
    classBox->clear();
}

void ShipSelect::classBoxHinted(const QStringList &types) {
    classBox->clear();
    classBox->addItems(types);
}
