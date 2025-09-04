#include "equipselect.h"
#include <QHBoxLayout>
#include "../../Protocol/equiptype.h"
#include "../clientv2.h"

extern std::unique_ptr<QSettings> settings;

EquipSelect::EquipSelect(int height, QWidget *parent)
    : height(height), QWidget{parent}
{
    searchLabel = new QLabel(this);
    //% "Search:"
    searchLabel->setText(qtTrId("equipview-search"));
    searchBox = new QLineEdit(this);
    searchBox->setObjectName("searchbox");
    searchBox->setMinimumSize(QSize(100, height));
    typeLabel = new QLabel(this);
    //% "Equip type:"
    typeLabel->setText(qtTrId("equipview-type"));
    typeBox = new QComboBox(this);
    typeBox->setObjectName("typeselect");
    /*
    typebox->setStyleSheet(
        "QComboBox#typeselect { color: palette(base); }");
*/
    equipLabel = new QLabel(this);
    //% "Equip:"
    equipLabel->setText(qtTrId("equipview-equip"));
    equipBox = new QComboBox(this);
    equipBox->setObjectName("equipdef");

    destructButton = new QPushButton(this);
    addStarButton = new QPushButton(this);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(searchLabel);
    layout->addWidget(searchBox);
    layout->addWidget(typeLabel);
    layout->addWidget(typeBox);
    layout->addWidget(equipLabel);
    layout->addWidget(equipBox);
    layout->addWidget(destructButton);
    layout->addWidget(addStarButton);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(6);
    layout->setAlignment(Qt::AlignCenter);

    //% "Destruct"
    destructButton->setText(qtTrId("destruct-button"));
    //% "Improve"
    addStarButton->setText(qtTrId("add-star-button"));


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
    searchBox->setMinimumSize(QSize(150, height));

    typeLabel->setSizePolicy(labelSize);
    typeBox->setSizePolicy(QSizePolicy(QSizePolicy::Maximum,
                                       QSizePolicy::Preferred,
                                       QSizePolicy::ComboBox));
    typeBox->resize(QSize(100, height));
    typeBox->addItem(qtTrId("all-equipments"));
    typeBox->addItems(EquipType::getDisplayGroupsSorted());

    equipLabel->setSizePolicy(labelSize);
    QSizePolicy equipBoxSize = QSizePolicy(QSizePolicy::Maximum,
                                           QSizePolicy::Preferred,
                                           QSizePolicy::ComboBox);
    equipBox->setSizePolicy(equipBoxSize);
    equipBox->resize(QSize(100, height));
    equipBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);

    destructButton->setSizePolicy(QSizePolicy(QSizePolicy::Maximum,
                                              QSizePolicy::Preferred,
                                              QSizePolicy::PushButton));
    destructButton->resize(QSize(100, height));

    addStarButton->setSizePolicy(QSizePolicy(QSizePolicy::Maximum,
                                             QSizePolicy::Preferred,
                                             QSizePolicy::PushButton));
    addStarButton->resize(QSize(100, height));

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
}

void EquipSelect::reCalculateAvailableEquips(int index) {
    Q_UNUSED(index);
    equipBox->clear();
    for(auto &equipReg:
         Clientv2::getInstance().equipRegistryCache) {
        if(
            (typeBox->currentText().compare("All equipments") == 0
             && equipReg->type.getDisplayGroup()
                        .compare("VIRTUAL", Qt::CaseInsensitive) != 0
             && !equipReg->localNames.value("ja_JP").isEmpty())
            || equipReg->type.getDisplayGroup()
                       .compare(typeBox->currentText(),
                                Qt::CaseInsensitive) == 0) {
            QString equipName = equipReg->toString(
                settings->value("language", "ja_JP").toString());
            if(equipName.isEmpty()) {
                equipName = equipReg->toString("ja_JP");
            }
            equipBox->addItem(equipName);
        }
    }
}
