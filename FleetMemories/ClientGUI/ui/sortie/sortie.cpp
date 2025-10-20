#include "sortie.h"
#include "ui_sortie.h"
#include <QTimer>
#include <QLabel>
#include <QResizeEvent>
#include <QPainter>
#include "maprender.h"
#include "../../clientv2.h"
extern std::unique_ptr<QSettings> settings;

Sortie::Sortie(QWidget *parent)
    : QFrame(parent)
    , ui(new Ui::Sortie)
{
    ui->setupUi(this);

    renderer = new MapRender(this);

    globeFrame = new MapViewWidget(renderer,
                                   MapRender::globeMapWidth,
                                   MapRender::globeMapHeight,
                                   ui->MapView);
    connect(renderer, &MapRender::mapSelected,
            this, &Sortie::switchMap);
    ui->DiffChoice->setSizeAdjustPolicy(QComboBox::AdjustToContents);
}

Sortie::~Sortie()
{
    delete ui;
}

void Sortie::setState(KP::SortieState state) {
    sortieState = state;
}

void Sortie::switchToState() {
    switch(sortieState) {
    case KP::MapView:
        ui->DiffChoice->clear();
        //% "Early"
        ui->DiffChoice->addItem(qtTrId("diff-c"));
        //% "Medium"
        ui->DiffChoice->addItem(qtTrId("diff-b"));
        //% "Late"
        ui->DiffChoice->addItem(qtTrId("diff-a"));
        ui->MapSelect->show();
        ui->BattleScreen->hide();
        update();
        break;
    default:
        break;
    }
}

void Sortie::resizeEvent(QResizeEvent *event) {
    globeFrame->resize(ui->MapView->size());

    QWidget::resizeEvent(event);
}

void Sortie::switchMap(int mapId) {
    Clientv2 &engine = Clientv2::getInstance();
    if(!engine.mapRegistryCacheGood) {
        return;
    }
    ui->DiffChoice->clear();
    auto meta = QMetaEnum::fromType<KP::Difficulty>();
    for(int i = 0; i < meta.keyCount(); ++i) {
        KP::Difficulty diff = static_cast<KP::Difficulty>(meta.value(i));
        if(engine.mapRegistryCache.contains(mapId + diff * KP::mapIDDifficultyMask)) {
            ui->selectDisplay->setText(engine.mapRegistryCache
                                           [mapId + diff * KP::mapIDDifficultyMask]
                                               ->toString(settings->value("client/language", "ja_JP")
                                                              .toString()));
            switch(diff)
            {
            case KP::EarlyWar:
                ui->DiffChoice->addItem(qtTrId("diff-c"));
                break;
            case KP::MidWar:
                ui->DiffChoice->addItem(qtTrId("diff-b"));
                break;
            case KP::LateWar:
                ui->DiffChoice->addItem(qtTrId("diff-a"));
                break;
            case KP::Historical:
                //% "Historical"
                ui->DiffChoice->addItem(qtTrId("diff-s"));
                break;
            }
        }
    }
}
