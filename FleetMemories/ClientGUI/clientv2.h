/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef CLIENTV2_H
#define CLIENTV2_H

#include <QColor>
#include <QtNetwork>
#include <QHttpServer>
#include "ui/factory/developwindow.h"
#include "ui/factory/constructwindow.h"
#include "ui/techview.h"
#include "ui/views/equipselect.h"
#include "model/equipmodel.h"
#include "model/specequipmodel.h"
#include "model/specshipmodel.h"
#include "model/shipmodel.h"
#include "model/shipbpmodel.h"
#include "model/shipdefmodel.h"
#include "model/localeawaresort.h"
#include "../Protocol/kp.h"
#include "../Protocol/receiver.h"
#include "../Protocol/sender.h"
#include "../Protocol/mapwithdiff.h"
#include "resourcefetch.h"
#include "steamauth.h"
#include "../Protocol/lua.h"

void customMessageHandler(QtMsgType,
                          const QMessageLogContext &,
                          const QString &);


class Clientv2 : public QObject {
    Q_OBJECT

public:
    virtual ~Clientv2() noexcept;

    static Clientv2 & getInstance() {
        static Clientv2 instance;
        return instance;
    }

    enum Password{
        normal,
        login,
        registering,
        confirm
    };
    Q_ENUM(Password);
    void enterBattle();
    bool isInBattle() const;
    bool isEquipRegistryCacheGood() const;
    bool isShipRegistryCacheGood() const;
    void leaveBattle();
    bool loggedIn() const;
    void doFetch(const QStringList &);

    /* ususally accesses equipregistryCache */
    friend void ConstructWindow::switchDisplay(int);
    friend void ConstructWindow::shipNameChanged(int);
    friend int DevelopWindow::equipIdDesired();
    friend void DevelopWindow::resetListName(int);
    friend void EquipSelect::reCalculateAvailableEquips(int);
    friend void ShipDefModel::addShips(QList<int>);
    friend void TechView::demandLocalTech(int);
    friend void TechView::demandSkillPoints(int);
    friend void TechView::resetLocalListName();
    friend void TechView::equipOrShip();

    int equipBigTypeIndex = 0;
    int equipIndex = 0;
    EquipModel equipModel;
    QList<SpecEquipModel *> specModels = QList<SpecEquipModel *>({
        new SpecEquipModel(&equipModel),
        new SpecEquipModel(&equipModel),
        new SpecEquipModel(&equipModel),
        new SpecEquipModel(&equipModel),
        new SpecEquipModel(&equipModel)
    });
    ShipModel shipModel;
    ShipBPModel shipBPModel;
    ShipDefModel shipDefModel;
    SpecShipModel *shipRemodelModel = new SpecShipModel(&shipModel);
    QSortFilterProxyModel *proxyModel = new LocaleAwareSort(this);
    QMap<int, double> techCache;

    sol::state lua;

public slots:
    void autoPassword();
    void backToNavalBase();
    void catbomb();
    void chooseHomePort(KP::AllegianceGroup);
    void demandEquipCache();
    void demandEquipSkillPoints(int);
    void demandMapCache();
    void demandMapSupremacy();
    void demandShipCache();
    void displayPrompt();
    void doBattle(const QJsonObject &);
    void doConstructShip(int shipDef, const QList<QUuid> &defaultEquips,
                         QUuid shipToRemodel, int factoryID);
    void doDestructEquip(const QList<QUuid> &);
    void doForceFetch(int);
    void doForceRepair(int);
    void doImproveEquip(const QList<QUuid> &);
    void doModernizeShip(const QList<QUuid> &);
    void doRefreshDock();
    void doRefreshFactory();
    void doRefreshFactoryAnchorage();
    void doRefreshFactoryArsenal();
    void doRepair(const QUuid &, int);
    void doStopRepair(int);
    Equipment * getEquipmentReg(int);
    Ship * getShipReg(int);
    bool parse(const QString &);
    void parseDisconnectReq();
    void parseQuit();
    bool parseSpec(const QStringList &);
    void queryNextNode(int mapId, int prevNode, bool retreat = false);
    void sendEncryptedAppTicket(uint8 [], uint32);
    void sendFleetData(const QJsonArray &);
    void serverResponse(const QString &, const QByteArray &);
    void serverResponseStd(const QJsonObject &);
    void serverResponseNonStd(const QByteArray &);
    void setTicketCache(uint8 [], uint32);
    void showHelp(const QStringList &);
    void sortie(int, int, bool expedition = false);
    void stopRepair(int);
    void switchToBattleView();
    void switchToFactory();
    void switchToFleetView();
    void switchToRepairView();
    void switchToTech();
    void switchToTech2();
    void switchToTech3(int);
    void tsunkitAssets();
    void tsunkitAssets2();
    void uiRefresh();
    Q_DECL_DEPRECATED void update();

signals:
    void aboutToQuit();
    void askForHomePort(const QJsonObject &);
    void battleEnd();
    void battleProcess(const QJsonObject &);
    void equipRegistryComplete();
    void gamestateChanged(KP::GameState);
    void lockBattle();
    void mapEnd();
    void mapRegistryComplete();
    void mapSupremacyChanged();
    void progressToNode(const MapNode &, int);
    void qout(QString, QColor background = QColor("white"),
              QColor foreground = QColor("black"));
    void receivedAnchorageShip(const QJsonObject &);
    void receivedArsenalEquip(const QJsonObject &);
    void receivedFactoryRefresh(const QJsonObject &);
    void receivedGlobalTechInfo(const QJsonObject &);
    void receivedGlobalTechInfo2(const QJsonObject &);
    void receivedLocalTechInfo(const QJsonObject &);
    void receivedLocalTechInfo2(const QJsonObject &);
    void receivedMapStart(const QJsonObject &);
    void receivedRepairRefresh(const QJsonObject &);
    void receivedResourceInfo(const QJsonObject &);
    void receivedShipBlueprint(const QJsonObject &);
    void receivedSkillPointInfo(const QJsonObject &);
    void shipRegistryComplete();
    void tsunkitAssetsComplete();
    void uiRefreshSig();
    void unlockBattle();

private slots:
    void changeGameState(KP::GameState);
    void encrypted();
    void errorOccurred(QAbstractSocket::SocketError);
    void errorOccurredStr(const QString &);
    void handshakeInterrupted(const QSslError &);
    void migrate(const QJsonObject &);
    void pskRequired(QSslPreSharedKeyAuthenticator *);
    void readyRead();
    void sendEATActual();
    void shutdown();

private:
    void doAddEquip(const QStringList &);
    void doDevelop(const QStringList &);
    void doDeleteTestEquip();
    void doDeleteTestShip();
    void doGenerateTestEquip();
    void doGenerateTestShip();
    void doSwitch(const QStringList &);
    void exitGracefully();
    void exitGraceSpec();
    QString gameStateString() const;
    static const QStringList getCommands();
    const QStringList getCommandsSpec() const;
    int getConsoleWidth();
    const QStringList getValidCommands() const;
    void invalidCommand();
    bool loginCheck();
    void luaInitEquipable();
    void parseConnectReq(const QStringList &);
    bool parseGameCommands(const QString &, const QStringList &);
    void qls(const QStringList &);
    void readWhenConnected(const QByteArray &);
    void readWhenUnConnected(const QByteArray &);
    void receivedAuth(const QJsonObject &);
    void receivedInfo(const QJsonObject &);
    void receivedLogin(const QJsonObject &);
    void receivedLogout(const QJsonObject &);
    void receivedMsg(const QJsonObject &);
    void receivedNewLogin(const QJsonObject &);
    void sendTestMessages();
    void showCommands(bool);
    void switchCert(const QStringList &);
    void updateEquipCache(const QJsonObject &);
    void updateMapCache(const QJsonObject &);
    void updateShipCache(const QJsonObject &);

    explicit Clientv2(QObject * parent = nullptr);

    QHostAddress address;
    quint16 port;

    QSslSocket socket;
    QSslConfiguration conf;
    Receiver recv;
    QPointer<Sender> sender;

    unsigned int maxRetransmit;
    unsigned int retransmitTimes = 0;

    QString clientName;
    QString serverName;

    bool attemptMode;
    bool logoutPending;

    KP::GameState gameState;

    QMap<int, Equipment *> equipRegistryCache;
    bool equipRegistryCacheGood = false;
    QMap<int, Ship *> shipRegistryCache;
    bool shipRegistryCacheGood = false;
public:
    QMap<int, MapWithDiff *> mapRegistryCache;
    bool mapRegistryCacheGood = false;
    QMap<int, double> mapSupremacies;
private:
    QHttpServer migrateServer;
    QTcpServer *tcpServer = new QTcpServer();

    ResourceFetch resourceFetcher;
    int downloadCompleted = 0;
    int downloadRequired = 0;

#pragma message(SALT_FISH)
    const QByteArray defaultSalt =
        QByteArrayLiteral("\xe8\xbf\x99\xe6\x98\xaf\xe4\xb8"
                          "\x80\xe6\x9d\xa1\xe5\x92\xb8\xe9"
                          "\xb1\xbc");

    SteamAuth sauth;
    QByteArray authCache;
    bool authSent = false;

    QTimer *timer;

    Q_DISABLE_COPY_MOVE(Clientv2)
};

#endif // CLIENTV2_H
