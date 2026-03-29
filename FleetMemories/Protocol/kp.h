/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef KP_H
#define KP_H

#define M_CONST \
    __FILE__ QT_STRINGIFY(:__LINE__: MAGICCONSTANT UNDESIREABLE NO 1)
#define NOT_M_CONST \
    __FILE__ QT_STRINGIFY(:__LINE__: This is considered an integral \
part of the program rather than magic constants.)
#define SALT_FISH __FILE__ QT_STRINGIFY(:__LINE__: This is a salt fish.)
#define SECRET \
    __FILE__ QT_STRINGIFY(:__LINE__: Go make your own steam app if modding!)
#define USED_CXX17 \
    __FILE__ QT_STRINGIFY(:__LINE__: This part uses C++ 17 features. \
Use macro "__cplusplus" to check whether your compiler supports it.)
#define USED_CXX23 \
    __FILE__ QT_STRINGIFY(:__LINE__: This part uses C++ 23 features. \
Use macro "__cplusplus" to check whether your compiler supports it.)
#define PRODUCTION_ENV 0

#include <QObject>
#include <QJsonObject>
#include <QCborValue>
#include <QDateTime>
#include <QLocale>
#include "steam/steamtypes.h"

#pragma message(NOT_M_CONST)
namespace {
const int steamRateLimit = 60; // see ClientGUI/steamauth.cpp
}
using TechEntry = std::tuple<QUuid, int, double>;

class ResOrd;

/* OS Specific */
#if defined (Q_OS_WIN)
// apparently some stupid win header <minwindef.h> interferes with std::max
#define NOMINMAX
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#elif defined (Q_OS_UNIX)
#include <locale.h>
#endif

/* "Kancolle Protocol */
namespace KP {
Q_NAMESPACE

/* Not in settings, because these values usually
 * have to be respected by both server and client */
#pragma message(NOT_M_CONST)
/* this is deliberately not customized */
/* do not modify as this is used in steam tickets */
static constexpr int practicalBufferSize = 1024;
static constexpr int fleetsSize = 4;
static constexpr int normalFleetSize = 7;
static constexpr int combinedFleetSize = 14;
static constexpr int fleetRepSize = 0x10;
static constexpr int disabledShip = -2;
static constexpr int maxEquipSlots = 5;
static constexpr int levelUnlockExSlot = 50;
static constexpr int normalFleetMaxCapitalness = 20;
static constexpr int combinedFleetMinCapitalness = 15;
static constexpr int combinedFleetMaxCapitalness = 50;
static constexpr int transportFleetMaxCapitalness = 25;
static constexpr int resourceMapIDStart = 1024;
static constexpr int resourceMapIDEnd = 2048;
static constexpr int mapIDDifficultyMask = 4096;
static constexpr int hiddenMap = 99;
static constexpr qint64 secsinMin = 60;
static constexpr int equipIdMax = 0x10000;
static constexpr int conditionMax = 480;
static constexpr int factorySlotRows = 6;
static constexpr int factorySlotColumns = 4;
static constexpr int maxRepairSlots = 8;
/* ARD coupon stored as integer units; 1 unit = 0.01 HKD */
static constexpr const char *attrARDCoupon = "ARDCoupon";
static constexpr const char *attrMedal = "Medal";
static constexpr double ardCouponUnitHKD = 0.01;
static constexpr int ardCouponMaxUnits = 65536;
static constexpr int medalCostPerUnit = 999;
#pragma message(SECRET)
static constexpr int ardCouponItemId = 6;
/* Returns the real price in HKD cents after logarithmic volume discount */
int ardRealPriceHKDCents(int units);
#if PRODUCTION_ENV
static constexpr const char *microTxnBaseUrl =
    "https://partner.steam-api.com/ISteamMicroTxn/";
#else
static constexpr const char *microTxnBaseUrl =
    "https://partner.steam-api.com/ISteamMicroTxnSandbox/";
#endif
#pragma message(NOT_M_CONST)
const int steamAppId = 2632870; // Go request your own steam appid if modding!
Q_GLOBAL_STATIC(QStringList,
                supportedLangs,
                QStringList(
                    {
                        "ja_JP",
                        "zh_CN",
                        "en_US"
                    }))

enum DgramType{
    Auth,
    Info,
    Message,
    Request,
};
Q_ENUM_NS(DgramType)

enum AuthMode{
    Logout,
    NewLogin, // Login
    Reg, // Register
};
Q_ENUM_NS(AuthMode)

enum LogoutType{
    LoggedElsewhere,
    LogoutFailure,
    LogoutSuccess,
    ViolatedRateLimit
};
Q_ENUM_NS(LogoutType)

enum MsgType{
    AccessDenied,
    AllowClientStart,
    AllowClientFinish,
    AskForHomePort,
    BattleError,
    BattleProcess,
    ConstructStart,
    DevelopFailed,
    DevelopStart,
    DisableShip,
    EquipImproved,
    EquipRetired,
    FairyBusy,
    FleetFail,
    Hello,
    LackPrivate,
    JsonError,
    MessageTestServer,
    NewEquip,
    NewShip,
    Penguin,
    ResourceRequired,
    ShipBPAdded,
    ShipBPRetired,
    ShipModernized,
    ShipRemodeled,
    Success,
    Unsupported,
    VerifyComplete,
    ARDPurchaseClawback,
    ARDPurchaseFailed,
    ARDPurchasePending,
    ARDPurchaseSuccess,
    MedalPurchased,
};
Q_ENUM_NS(MsgType)

enum GameState{
    Offline,
    Port,
    Factory,
    TechView,
    SortieMapView,
    FleetView,
    BattleMapView,
    RepairView,
};
Q_ENUM_NS(GameState)

/* 3-resources.md#Base types */
enum ResourceType{
    O, // oil
    E, // Explosives
    S, // Steel,
    R, // Rubber,
    A, // Aluminium,
    W, // Tungsten,
    C, // Chromium
};
Q_ENUM_NS(ResourceType)

enum CommandType{
    ChangeState,
    Develop,
    Fetch,
    Refresh,
    SteamAuth,
    SteamLogout,
    CHello,
    Adminaddequip,
    Admingenerateequips,
    Adminremoveequips,
    Admingenerateships,
    Adminremoveships,
    DemandEquipInfo,
    DemandEquipInfoUser,
    DemandShipInfo,
    DemandShipInfoUser,
    DemandMapInfo,
    DemandMapInfoUser,
    Repair,
    DemandResourceUpdate,
    DemandSkillPoints,
    DemandTech,
    DestructEquip,
    Switch,
    MessageTest,
    Migrate,
    FleetData,
    SelectHomePort,
    Modernize,
    Construct,
    RequestSortie,
    ProgressMap,
    EnterBattleNode,
    ChooseNode,
    DemandRankInfo,
    ARDPurchaseAuth,
    BuyFromStore,
    BuyMedal,
    InitARDPurchase,
};
Q_ENUM_NS(CommandType)

enum GameError{
    ResourceLack,
    DevelopNotOption,
    DevelopNotExist,
    FactoryNotOpen,
    FactoryBusy,
    MassProductionDisallowed,
    ProductionDisallowed,
    CloningDisallowed,
    DefaultEquipIncorrect,
    RemodelShipIncorrect,
    BlueprintNonexistent,
    IndustrialPointsLack,
    ShipisDisabled,
    ShipisUnderRepair,
    FleetBusy,
    FleetLost,
    ServerError,
    DropError
};
Q_ENUM_NS(GameError)

enum FactoryState{
    Development,
    Construction,
    Arsenal,
    Anchorage,
    BlueprintView,
    RankView,
};
Q_ENUM_NS(FactoryState)

enum SortieState{
    MapView,
    MapDetail,
    DrillView,
    BattleScreen
};
Q_ENUM_NS(SortieState)

enum BattleState{
    NoBattle,
    BeforeBattle,
    DuringBattle,
    AfterBattle,
};
Q_ENUM_NS(BattleState)

enum InfoType{
    FactoryInfo,
    DockInfo,
    EquipInfo,
    EquipInfoUser,
    GlobalTechInfo,
    LocalTechInfo,
    SkillPointInfo,
    ResourceInfo,
    RankInfo,
    ShipInfo,
    ShipInfoUser,
    ShipInfoUserBP,
    MapInfo,
    MapInfoUser,
    MapStart,
    MapProgress,
};
Q_ENUM_NS(InfoType)

enum AuthFailType{
    TicketFailedToDecrypt,
    TicketIsntFromCorrectAppID,
    RequestTimeout,
    SteamIdInvalid,
    SteamAuthFail
};
Q_ENUM_NS(AuthFailType)

enum PurchaseFailReason{
    PurchaseNotAuthorized,
    PurchaseOrderNotFound,
    PurchaseOrderMismatch,
    PurchaseDatabaseError,
    PurchaseInvalidAmount,
    PurchaseSteamError,
    PurchaseEquipNotExist,
    PurchaseEquipNotAvailable,
    PurchaseInsufficientCoupons,
};
Q_ENUM_NS(PurchaseFailReason)

enum FleetFailType{
    ValidFleet,
    FleetSizeError,
    FleetTypeError,
    EquipError,
    FleetContainsDisabled,
    FleetDontFitMap,
    FleetBusyInBattle,
    FleetShipisUnderRepair,
};
Q_ENUM_NS(FleetFailType)

enum ConsoleCommandType{
    Help,
    Exit,
    Commands,
    Allcommands,
    Connect,
    Disconnect,
    Switchcert,
    Messagetest
};
Q_ENUM_NS(ConsoleCommandType)

enum AllegianceGroup{
    UnknownNation = 0x0,
    Japanese = 0x1,
    German = 0x2,
    Italian = 0x3,
    American = 0x4,
    British = 0x5,
    French = 0x6,
    Soviet = 0x7, // includes any non-Baltic Soviet Republic
    Chinese = 0x8,
    Benelux = 0x9,
    Nordic = 0xA,
    /* British control in WWII, not actual membership in current Commonwealth */
    Commonwealth = 0xB,
    // Iberian or Latin American, or Spanish and Portuguese controlled
    // territories in WWII elsewhere
    Latino = 0xC,
    // Some Western/Central European countries such as Switzerland get
    // lumped here, but all are landlocked
    EasternEuropean = 0xD,
    MinorAsian = 0xE,
    Fantasy = 0xF
};
Q_ENUM_NS(AllegianceGroup)

enum AllegianceSubGroup{
    DUnknownNation = 0x00,
    DJapanese = 0x10,
    /* Ships named after islands that have huge distance to Japanese mainland */
    DJapaneseOutlying = 0x1D,
    /* Ships named after places that are not treated as Japanese
     * by Allied nations */
    DJapaneseExterior = 0x1E,
    /* Ships named after places in Okinawa Pref. or southernmost
     * islands in Kagoshima Pref.
     * (Does not prevent said ships being counted as Japanese
     * anywhere in game rules) */
    DRyukyuan = 0x1F,
    DGerman = 0x20,
    /* Austria-connected LBAS or WWI Austro-Hungarian navy
     * (that did not qualify for other subgroups) might go here */
    DAustrian = 0x2C,
    DItalian = 0x30,
    DAlbanian = 0x3A,
    /* Italian colonies and subsequent independent counties;
     * not other East African countries */
    DEastAfrican = 0x3E,
    DLibyan = 0x3F,
    DAmerican = 0x40,
    /* American unincorporated territories, states that have COFA with
     * USA, as well as Liberia goes here */
    DAmericanAssociates = 0x4E,
    DFilipino = 0x4F,
    /* have a whole 0xB? space for British Empire territories in WWII */
    DBritish = 0x50,
    DFrench = 0x60,
    DIndochinese = 0x68,
    DAlgerian = 0x6A,
    DMoroccoan = 0x6B,
    DTunisian = 0x6C,
    DMauritanian = 0x6D,
    DOtherFrancophone = 0x6E,
    DSovietOrRussian = 0x70,
    DUkrainian = 0x7C,
    /* Except from Russian and Ukrainian above, all founding members of
     * CIS is included ignoring any later withdrawal */
    DCIS = 0x7E,
    DChineseNationalist = 0x80, /* Nanjing/Taipei regime goes here */
    DChineseMonarchical = 0x86, /* Qing/"Manchukuo" goes here */
    DMongolian = 0x87,
    DChineseModern = 0x88, /* Beijing regime goes here */
    DChineseOther = 0x8F, /* Hong Kong and Macau before 1997/1999 goes here */
    DIndonesian = 0x90,
    DDutch = 0x98,
    DDutchSpeakingAmericas = 0x9A,
    DBelgian = 0x9B,
    /* Belgian colonies and subsequent independent counties;
     * not other Central African contries */
    DCentralAfrican = 0x9D,
    DBeneluxOther = 0x9F,
    DSwedish = 0xA0,
    DDanish = 0xA4,
    DDanishKingdom = 0xA7,
    DNorwegian = 0xA8,
    DIcelandic = 0xAB,
    DFinnish = 0xAC,
    DAustralian = 0xB0,
    DNewZealander = 0xB2,
    /* A lot of this is/was external territories of Australia/New Zealand */
    DOceanaian = 0xB3,
    DSouthAfricanOrNamibian = 0xB4,
    DIrish = 0xB5, /* for linguistic reasons */
    DMalaysianOrBruneian = 0xB6,
    DSingaporean = 0xB7,
    DIndian = 0xB8,
    DPakistani = 0xBA,
    DBangladeshi = 0xBB,
    DCanadian = 0xBC,
    DEgyptian = 0xBE,
    /* British control in WWII, not actual membership in current Commonwealth */
    DOtherCommonwealth = 0xBF,
    DSpanish = 0xC0,
    DPortuguese = 0xC2,
    DBrazilian = 0xC4,
    DArgentinian = 0xC6,
    DPeruvian = 0xC8,
    DChilean = 0xC9,
    DMexican = 0xCA,
    DCuban = 0xCB,
    DColumbianOrEcuadoran = 0xCC,
    DVenezuelan = 0xCD,
    DOtherLatinAmerican = 0xCE,
    /* Spanish and Portuguese controlled territories at the time of
     * WWII outside Latin America */
    DOtherLatino = 0xCF,
    DYugoslavian = 0xD0,
    DPolish = 0xD2,
    DBulgarian = 0xD4,
    DGreekOrCypriot = 0xD6,
    DRomanian = 0xD8,
    DTurkish = 0xDA,
    DBaltic = 0xDC,
    /* Not 0xEC for it does not participate in many Asian
     * organizations, like Armenia */
    DIsraeli = 0xDE,
    DOtherEuropean = 0xDF,
    DThai = 0xE0,
    DIranian = 0xE2,
    DArabicAsian = 0xE4,
    DSouthKorean = 0xE8,
    DNorthKorean = 0xEA,
    DOtherAsian = 0xEC,
    DFantasy = 0xF0,
};
Q_ENUM_NS(AllegianceSubGroup)

Q_GLOBAL_STATIC(QStringList,
                nationLiteral,
                QStringList(
                    {
                        //% "Japanese"
                        QT_TRID_NOOP("Japanese"),
                        //% "German"
                        QT_TRID_NOOP("German"),
                        //% "Italian"
                        QT_TRID_NOOP("Italian"),
                        //% "American"
                        QT_TRID_NOOP("American"),
                        //% "British"
                        QT_TRID_NOOP("British"),
                        //% "French"
                        QT_TRID_NOOP("French"),
                        //% "Soviet"
                        QT_TRID_NOOP("Soviet"),
                        //% "Chinese"
                        QT_TRID_NOOP("Chinese"),
                        //% "Benelux"
                        QT_TRID_NOOP("Benelux"),
                        //% "Nordic"
                        QT_TRID_NOOP("Nordic"),
                        //% "Commonwealth"
                        QT_TRID_NOOP("Commonwealth"),
                        //% "Iberian/Latin American"
                        QT_TRID_NOOP("Latin"),
                        //% "Eastern European"
                        QT_TRID_NOOP("EasternEuropean"),
                        //% "Other Asian"
                        QT_TRID_NOOP("MinorAsian"),
                        //% "Fantasy ships"
                        QT_TRID_NOOP("Fantasy"),
                    }))

enum FleetType {
    NormalFleet = 0,
    CarrierFleet = 1,
    SurfaceFleet = 2,
    TransportFleet = 3
};
Q_ENUM_NS(FleetType);

enum CapitalType {
    Screen = 0,
    BattleShip = 1,
    Carrier = 2,
    OtherShip = 3
};

/* must be consistent with canequip.lua */
enum EquipSpecial{
    NonSpecial = 0,
    MidgetSub = 1,
    DepthCharge = 2,
    Smoke = 3,
    Sonar = 4,
    Ballon = 5,
    APShell = 6,
    AntilandShell = 7,
    AntilandRocket = 8,
    LandingCraft = 9,
    LandingTank = 10,
    Drum = 11,
    TPMaterial = 12,
    EngineTurbine = 13,
    EngineBoiler = 14,
    SearchLight = 15,
    Starshell = 16,
    RepairItem = 17,
    UnderwayReplenish = 18,
    Food = 19,
    CommandFacility = 20,
    AircraftPersonnel = 21,
    RepairFacility = 22,
    SurfacePersonnel = 23,
    LimitedNightPlane = 24,
    AntiAir = 25,
    FlyingBoat = 26,
    LBInterceptor = 27,
    JetPlane = 28,
    Bulge = 29,
    AAControl = 30,
    LandCorps = 31
};
Q_ENUM_NS(EquipSpecial)

enum Difficulty {
    EarlyWar = 0,
    MidWar = 1,
    LateWar = 2,
    Historical = 3
};
Q_ENUM_NS(Difficulty)

/* 6.1-maps.md#Battle node types */
/* must ensure consistency with maps.lua */
enum NodeType {
    STARTING = 0,
    NORMAL = 1,
    BOSS = 2,
    EMPTY = 3,
    DISASTER = 4,
    NIGHT = 5,
    NIGHTBOSS = 6,
    AIR = 7,
    TRANSPORT = 8,
    CHOICE = 9,
};
Q_ENUM_NS(NodeType)

enum BattleAssessment {
    SVictory = 0,
    AVictory = 1,
    BVictory = 2,
    CDefeat = 3,
    DDefeat = 4,
    EDefeat = 5
};
Q_ENUM_NS(BattleAssessment)

using DiffMap = QMap<Difficulty, QString>;
Q_GLOBAL_STATIC(DiffMap,
                diffEnumtoStr,
                DiffMap(
                    {
                        {EarlyWar, "C"},
                        {MidWar, "B"},
                        {LateWar, "A"},
                        {Historical, "H"}
                    }
                    ))

Q_GLOBAL_STATIC(QStringList,
                fleetTypes,
                QStringList(
                    {
                        //% "Normal"
                        QT_TRID_NOOP("NormalFleet"),
                        //% "Carrier"
                        QT_TRID_NOOP("CarrierFleet"),
                        //% "Surface"
                        QT_TRID_NOOP("SurfaceFleet"),
                        //% "Transport"
                        QT_TRID_NOOP("TransportFleet"),
                    }))


void initLog(bool server = false);
#if defined (Q_OS_WIN)
void winConsoleCheck();
#endif

/* See JSON support in Qt, especially QCborValue */
QByteArray accessDenied();
QByteArray catbomb();
QByteArray clientAddEquip(int);
QByteArray clientAdminTestEquip();
QByteArray clientAdminTestEquipRemove();
QByteArray clientAdminTestShip();
QByteArray clientAdminTestShipRemove();
QByteArray clientARDPurchaseAuth(quint64 orderId, bool authorized);
QByteArray clientBuy(int);
QByteArray clientBuyFromStore(int equipDef);
QByteArray clientBuyMedal(int amount);
QByteArray clientChooseNode(int mapId, int chosenNodeId);
QByteArray clientConstruct(int,
                           const QList<QUuid> &,
                           QUuid &,
                           int);
QByteArray clientDemandDestructEquip(const QList<QUuid> &);
QByteArray clientDemandEquipInfo(QDateTime timeUtc
                                 = QDateTime(QDate(1970, 1, 1),
                                             QTime(0, 0, 0)));
QByteArray clientDemandEquipInfoUser();
QByteArray clientDemandMapInfo(QDateTime timeUtc
                               = QDateTime(QDate(1970, 1, 1),
                                           QTime(0, 0, 0)));
QByteArray clientDemandMapInfoUser();
QByteArray clientDemandModernize(const QList<QUuid> &, bool);
QByteArray clientDemandRankInfo(int, std::optional<int> page = std::nullopt);
QByteArray clientDemandRepair(const QUuid &, int,
                              bool stop = false, bool forced = false);
QByteArray clientDemandResourceUpdate();
QByteArray clientDemandShipInfo(QDateTime timeUtc
                                = QDateTime(QDate(1970, 1, 1),
                                            QTime(0, 0, 0)));
QByteArray clientDemandShipInfoUser();
QByteArray clientDemandSkillPoints(int);
QByteArray clientDemandTech(int local = 0);
QByteArray clientDevelop(int, bool convert = false, int factoryID = -1);
QByteArray clientDoBattleNode(const QJsonObject &);
QByteArray clientDockRefresh();
QByteArray clientFactoryRefresh();
QByteArray clientFetch(int factoryID = -1, bool forced = false);
QByteArray clientFleetData(const QJsonArray &);
QByteArray clientHello();
QByteArray clientHomePort(AllegianceGroup);
QByteArray clientInitARDPurchase(int units);
QByteArray clientMigrate(const QJsonObject &);
QByteArray clientQueryNextNode(int, int, bool retreat = false);
QByteArray clientSortie(int, int, bool);
QByteArray clientStateChange(GameState);
QByteArray clientSteamAuth(uint8 [], uint32);
QByteArray clientSteamLogout();
QByteArray clientTestMessages(int);

/* factoryslot, repairslot */
#pragma message(USED_CXX23)
static constexpr std::tuple<int, int> getDesiredSlots(int mapsOpened) {
    if(mapsOpened < 0) {
        return {0, 0};
    }
    static QMap<int, int> fact
        = { std::pair(0, 4),
            std::pair(4, 5),
            std::pair(6, 6),
            std::pair(8, 7),
            std::pair(10, 8),
            std::pair(12, 9),
            std::pair(15, 10),
            std::pair(19, 11),
            std::pair(23, 12),
            std::pair(27, 13),
            std::pair(32, 14),
            std::pair(37, 15),
            std::pair(42, 16),
            std::pair(47, 17),
            std::pair(52, 18),
            std::pair(57, 19),
            std::pair(62, 20),
            std::pair(68, 21),
            std::pair(74, 22),
            std::pair(80, 23),
            std::pair(86, 24)
        };
    static QMap<int, int> repair
        = { std::pair(0, 2),
            std::pair(4, 3),
            std::pair(8, 4),
            std::pair(15, 5),
            std::pair(23, 6),
            std::pair(42, 7),
            std::pair(86, 8)
        };
cal_factory:
    int factoryResult = 0;
    {
        int tempa = mapsOpened;
        do {
            if(fact.contains(tempa)) {
                factoryResult = fact[tempa];
                break;
            }
        } while(tempa--);
    }
cal_repair:
    int repairResult = 0;
    {
        int tempa = mapsOpened;
        do {
            if(repair.contains(tempa)) {
                repairResult = repair[tempa];
                break;
            }
        } while(tempa--);
    }
    return {factoryResult, repairResult};
}

static constexpr int initFactory() {
    return std::get<0>(getDesiredSlots(0));
}

static constexpr int initDock() {
    return std::get<1>(getDesiredSlots(0));
}

QByteArray serverARDPurchaseClawback(int unitsDeducted);
QByteArray serverARDPurchaseFailed(PurchaseFailReason reason);
QByteArray serverARDPurchasePending(quint64 orderId);
QByteArray serverARDPurchaseSuccess(int unitsAdded);
QByteArray serverAskForHomePort();
QByteArray serverBattleEnd();
QByteArray serverBattleError(GameError);
QByteArray serverBattleProcess(const QJsonObject &);
QByteArray serverBlueprintAdded(int);
QByteArray serverBlueprintRetired(int);
QByteArray serverDevelopFailed(GameError);
QByteArray serverDevelopStart(bool construct = false);
QByteArray serverDisableShip(QUuid);
QByteArray serverEquipLackFather(GameError, int);
QByteArray serverEquipLackMother(GameError, int, int64);
QByteArray serverEquipImproved(const QList<std::tuple<QUuid, int>> &);
QByteArray serverEquipRetired(const QList<QUuid> &);
QByteArray serverEquipInfo(const QJsonArray &, bool user = false,
                           QDateTime timeUtc = QDateTime::currentDateTimeUtc(),
                           bool cacheHit = false);
QByteArray serverFairyBusy(int);
QByteArray serverFleetFailure(FleetFailType);
QByteArray serverGlobalTech(double, int);
QByteArray serverGlobalTech(const QList<TechEntry> &, bool);
QByteArray serverHello();
QByteArray serverLackPrivate();
QByteArray serverLogFail(AuthFailType);
QByteArray serverLogSuccess(bool);
QByteArray serverLogout(LogoutType);
QByteArray serverMapInfo(const QJsonArray &,
                         QDateTime timeUtc = QDateTime::currentDateTimeUtc(),
                         bool cacheHit = false);
QByteArray serverMapInfoUser(const QJsonObject &);
QByteArray serverMapNotOpen(int mapId);
QByteArray serverMapProgress(int mapId, int nextNode);
QByteArray serverMapStart(int mapId, int startNode);
QByteArray serverMedalPurchased(int amount);
QByteArray serverNewEquip(QUuid, int);
QByteArray serverNewmodelShip(QUuid, int, int);
QByteArray serverNewShip(QUuid, int, int);
QByteArray serverParseError(MsgType, const QString &,
                            const QString &);
QByteArray serverPenguin();
QByteArray serverRankInfo(const QJsonArray &, int, std::optional<double>);
QByteArray serverResourceUpdate(ResOrd, int ardcoupon, int medal);
QByteArray serverShipBPInfo(const QJsonObject &);
QByteArray serverShipInfo(const QJsonArray &, bool user = false,
                          QDateTime timeUtc = QDateTime::currentDateTimeUtc(),
                          bool cacheHit = false);
QByteArray serverShipModernized(const QList<std::tuple<QUuid, int>> &);
QByteArray serverSkillPoints(int, int64, int64);
QByteArray serverSuccess();
QByteArray serverTestMessages(int);
QByteArray serverVerifyComplete();
QByteArray weighAnchor();

}

#endif // KP_H
