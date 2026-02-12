#include "constructwindow.h"
#include "../../clientv2.h"
#include "ui_constructwindow.h"
#include "../../../Protocol/kp.h"
#include <QMetaEnum>

extern std::unique_ptr<QSettings> settings;

ConstructWindow::ConstructWindow(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ConstructWindow)
{
    ui->setupUi(this);
    connect(ui->shipnation, &QComboBox::currentIndexChanged,
            this, &ConstructWindow::switchDisplay);
    connect(ui->shiptype, &QComboBox::currentIndexChanged,
            this, &ConstructWindow::switchDisplay);
    connect(ui->shipclass, &QComboBox::currentIndexChanged,
            this, &ConstructWindow::switchDisplay);
}

ConstructWindow::~ConstructWindow()
{
    disconnect(ui->shipnation, &QComboBox::currentIndexChanged,
               this, &ConstructWindow::switchDisplay);
    disconnect(ui->shiptype, &QComboBox::currentIndexChanged,
               this, &ConstructWindow::switchDisplay);
    disconnect(ui->shipclass, &QComboBox::currentIndexChanged,
               this, &ConstructWindow::switchDisplay);
    delete ui;
}

void ConstructWindow::switchDisplay(int) {
    const QString nationality
        = ui->shipnation->currentText().
                  localeAwareCompare(qtTrId("all-nationality")) == 0
              ? QLatin1String("") : ui->shipnation->currentText();
    const QString shiptype
        = ui->shiptype->currentText().
                  localeAwareCompare(qtTrId("all-shiptypes")) == 0
              ? QLatin1String("") : ui->shiptype->currentText();
    const QString shipclass
        = ui->shipclass->currentText().
                  localeAwareCompare(qtTrId("all-shipclasses")) == 0
              ? QLatin1String("") : ui->shipclass->currentText();
    const QString searchTerm = "";
    ui->shipname->clear();
    Clientv2 &engine = Clientv2::getInstance();
    /*
    if(!engine.shipBPModel.isReady()) {
        qCritical() << "FUCK";
        engine.doRefreshFactoryAnchorage();
        QTimer timer;
        timer.setSingleShot(true);
        QEventLoop loop;
        connect(&engine.shipBPModel, &ShipBPModel::bpReady, &loop, &QEventLoop::quit);
        connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timer.start(settings->value("networkclient/downloadwaittimemsec", 80000).toInt());
        loop.exec();
    }
    qCritical() << "FUCK2" << engine.shipBPModel.bpCache.size();
*/
    bool pass = true;
    bool pass1 = false;
    //% "All ship types"
    QStringList typePasses = {qtTrId("all-shiptypes")};
    //% "All ship classes"
    QStringList classPasses = {qtTrId("all-shipclasses")};;
    static auto meta = QMetaEnum::fromType<KP::ShipNationality>();
    for(auto iter = engine.shipBPModel.clientShipBPs.keyBegin();
         iter != engine.shipBPModel.clientShipBPs.keyEnd();
         ++iter) {
        Ship *ship = engine.shipRegistryCache[*iter];
        pass = true;
        if(!nationality.isEmpty() &&
            qtTrId(meta.key(ship->getNationality()))
                    .localeAwareCompare(nationality) != 0) {
            pass = false;
        }
        if(shiptype.isEmpty() && shipclass.isEmpty()) {
            if(pass) {
                QString type = ship->getType().toString();
                if(!typePasses.contains(type)
                    && type != qtTrId("all-shiptypes")) {
                    typePasses.append(type);
                }
            }
        }

        if(!shiptype.isEmpty() &&
            ship->getType().toString().localeAwareCompare(
                shiptype) != 0) {
            pass = false;
        }
        QString classText =
            ship->shipClassText[
                settings->value("client/language", "ja_JP").toString()
        ];
        if(classText.isEmpty()) {
            classText = ship->shipClassText["ja_JP"];
        }
        if(shipclass.isEmpty()) {
            if(pass) {
                if(!classPasses.contains(classText)
                    && classText != qtTrId("all-shipclasses")) {
                    classPasses.append(classText);
                }
            }
        }

        if(!shipclass.isEmpty() && classText.localeAwareCompare(
                                        shipclass) != 0) {
            pass = false;
        }
        if(pass) {
            ui->shipname->addItem(ship->toString());
        }
    }

    if(searchTerm.isEmpty() && shiptype.isEmpty() && shipclass.isEmpty()) {
        /* disconnect to eliminate infinite recursion */
        disconnect(ui->shiptype, &QComboBox::currentIndexChanged,
                   this, &ConstructWindow::switchDisplay);
        std::sort(typePasses.begin(), typePasses.end(), [](QString a, QString b)
                  { return a.localeAwareCompare(b) > 0; });
        ui->shiptype->clear();
        ui->shiptype->addItem(qtTrId("all-shiptypes"));
        ui->shiptype->addItems(typePasses);
        ui->shipclass->clear();
        update();
        connect(ui->shiptype, &QComboBox::currentIndexChanged,
                this, &ConstructWindow::switchDisplay);
    }

    if(searchTerm.isEmpty() && shipclass.isEmpty()) {
        disconnect(ui->shipclass, &QComboBox::currentIndexChanged,
                this, &ConstructWindow::switchDisplay);
        std::sort(classPasses.begin(), classPasses.end(), [](QString a, QString b)
                  { return a.localeAwareCompare(b) > 0; });
        ui->shipclass->clear();
        ui->shipclass->addItem(qtTrId("all-shipclasses"));
        ui->shipclass->addItems(classPasses);
        update();
        connect(ui->shipclass, &QComboBox::currentIndexChanged,
                this, &ConstructWindow::switchDisplay);
    }
}

void ConstructWindow::initialize() {
    auto meta = QMetaEnum::fromType<KP::ShipNationality>();
    for(int i = 0; i < meta.keyCount(); ++i) {
        if(meta.value(i) == KP::ShipNationality::UnknownNation) {
            ui->shipnation->addItem(qtTrId("all-nationality"));
        }
        else {
            ui->shipnation->addItem(qtTrId(meta.key(i)));
        }
    }
}
