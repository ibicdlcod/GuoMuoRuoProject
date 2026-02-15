#ifndef KP_H
#define KP_H

#define M_CONST __FILE__ QT_STRINGIFY(:__LINE__: MAGICCONSTANT UNDESIREABLE NO 1)
#define NOT_M_CONST __FILE__ QT_STRINGIFY(:__LINE__: This is considered an integral part of the program rather than magic constants.)
#define SALT_FISH __FILE__ QT_STRINGIFY(:__LINE__: This is a salt fish.)
#define SECRET __FILE__ QT_STRINGIFY(:__LINE__: Go make your own steam app if modding!)
#define USED_CXX17 __FILE__ QT_STRINGIFY(:__LINE__: This part uses C++ 17 features. Use macro "__cplusplus" to check whether your compiler supports it.)

#include <QObject>
#include <QJsonObject>
#include <QCborValue>
#include <QDateTime>
#include "steam/steamtypes.h"

#pragma message(NOT_M_CONST)
namespace {
const int steamRateLimit = 60; // see ClientGUI/steamauth.cpp
}
using TechEntry = std::tuple<QUuid, int, double>;

class ResOrd;

/* OS Specific */
#if defined (Q_OS_WIN)
#define NOMINMAX // apparently some stupid win header <minwindef.h> interferes with std::max
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

static constexpr int initDock = 2;
static constexpr int initFactory = 4;
static constexpr int fleetsSize = 4;
static constexpr int normalFleetSize = 7;
static constexpr int combinedFleetSize = 14;
static constexpr int fleetRepSize = 0x10;
static constexpr int maxEquipSlots = 5;
static constexpr int levelUnlockExSlot = 30;
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
#pragma message(NOT_M_CONST)
const int steamAppId = 2632870; // Go request your own steam appid if modding!
static const QStringList supportedLangs = {"ja_JP", "zh_CN", "en_US"};

enum DgramType{
    Auth,
    Message,
    Request,
    Info
};
Q_ENUM_NS(DgramType)

enum AuthMode{
    //Login,
    Reg,
    Logout,
    NewLogin
};
Q_ENUM_NS(AuthMode)

enum LogoutType{
    LoggedElsewhere,
    LogoutFailure,
    LogoutSuccess
};
Q_ENUM_NS(LogoutType)

enum MsgType{
    JsonError,
    Unsupported,
    AccessDenied,
    DevelopFailed,
    DevelopStart,
    ConstructStart,
    ResourceRequired,
    FairyBusy,
    Penguin,
    NewEquip,
    NewShip,
    Hello,
    LackPrivate,
    AllowClientStart,
    AllowClientFinish,
    VerifyComplete,
    EquipRetired,
    Success,
    MessageTestServer,
    FleetFail,
    AskForHomePort,
    ShipModernized,
    ShipBPRetired,
};
Q_ENUM_NS(MsgType)

enum GameState{
    Offline,
    Port,
    Factory,
    TechView,
    BattleView,
    FleetView
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
    DemandTech,
    DemandSkillPoints,
    DemandResourceUpdate,
    DestructEquip,
    Switch,
    MessageTest,
    Migrate,
    FleetData,
    SelectHomePort,
    ModernizeShip,
    Construct,
};
Q_ENUM_NS(CommandType)

enum GameError{
    ResourceLack,
    DevelopNotOption,
    DevelopNotExist,
    FactoryBusy,
    MassProductionDisallowed,
    ProductionDisallowed,
    CloningDisallowed,
    DefaultEquipIncorrect,
    RemodelShipIncorrect,
    BlueprintNonexistent,
};
Q_ENUM_NS(GameError)

enum FactoryState{
    Development,
    Construction,
    Arsenal,
    Anchorage,
    BlueprintView
};
Q_ENUM_NS(FactoryState)

enum SortieState{
    MapView,
    DrillView,
    BattleScreen
};
Q_ENUM_NS(SortieState)

enum InfoType{
    FactoryInfo,
    EquipInfo,
    EquipInfoUser,
    GlobalTechInfo,
    LocalTechInfo,
    SkillPointInfo,
    ResourceInfo,
    ShipInfo,
    ShipInfoUser,
    ShipInfoUserBP,
    MapInfo,
    MapInfoUser,
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

enum FleetFailType{
    ValidFleet,
    FleetSizeError,
    FleetTypeError,
    EquipError
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

enum ShipNationality{
    UnknownNation = 0,
    Japanese = 1,
    German = 2,
    Italian = 3,
    American = 4,
    British = 5,
    French = 6,
    Soviet = 7, // includes any non-Baltic Soviet Republic
    Chinese = 8,
    Benelux = 9,
    Nordic = 0xA,
    Commonwealth = 0xB,
    Latino = 0xC, // Iberian or Latin American
    EasternEuropean = 0xD,
    MinorAsian = 0xE,
    Fantasy = 0xF
};
Q_ENUM_NS(ShipNationality)

enum ShipNationalityDetailed{
    DUnknownNation = 0x00,
    DJapanese = 0x10,
    DGerman = 0x20,
    DItalian = 0x30,
    DItalianEastAfrican = 0x3E,
    DLibyan = 0x3F,
    DAmerican = 0x40,
    DFilipino = 0x4F,
    DBritish = 0x50,
    DFrench = 0x60,
    DFrenchIndochinese = 0x6E,
    DOtherFrancophone = 0x6F,
    DSovietRussian = 0x70,
    DUkrainian = 0x7C,
    DSovietNonRussian = 0x7E,
    DChineseNationalist = 0x80,
    DChineseModern = 0x88,
    DDutchEastIndian = 0x90,
    DDutch = 0x98,
    DDutchAmerican = 0x9A,
    DBelgian = 0x9B,
    DDRCongolese = 0x9D,
    DSwedish = 0xA0,
    DDanish = 0xA4,
    DNorwegian = 0xA8,
    DIcelandic = 0xAB,
    DFinnish = 0xAC,
    DAustralian = 0xB0,
    DNewZealander = 0xB2,
    DOceanaian = 0xB3,
    DSouthAfrican = 0xB4,
    DIrish = 0xB5,
    DMalaysian = 0xB6,
    DSingaporean = 0xB7,
    DIndian = 0xB8,
    DPakistani = 0xBA,
    DBangladeshi = 0xBB,
    DCanadian = 0xBC,
    DOtherCommonwealth = 0xBE,
    DSpanish = 0xC0,
    DPortuguese = 0xC2,
    DBrazilian = 0xC4,
    DArgentinian = 0xC6,
    DPeruvian = 0xC8,
    DChilean = 0xC9,
    DMexican = 0xCA,
    DColumbianOrEcuadoran = 0xCC,
    DVenezuelan = 0xCD,
    DCuban = 0xCE,
    DOtherLatino = 0xCF,
    DYugoslavian = 0xD0,
    DPolish = 0xD2,
    DBulgarian = 0xD4,
    DGreekOrCypriot = 0xD6,
    DRomanian = 0xD8,
    DTurkish = 0xDA,
    DBaltic = 0xDC,
    DAlbanian = 0xDD,
    DOtherEuropean = 0xDE,
    DThai = 0xE0,
    DIranian = 0xE2,
    DArabic = 0xE4,
    DSouthKorean = 0xE8,
    DNorthKorean = 0xEA,
    DOtherAsian = 0xEC,
    DFantasy = 0xF0,
};
Q_ENUM_NS(ShipNationalityDetailed)

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
                    }));

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
    EarlyWar,
    MidWar,
    LateWar,
    Historical
};
Q_ENUM_NS(Difficulty)

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
                    }));


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
QByteArray clientConstruct(int shipDef,
                           const QList<QUuid> &defaultEquips,
                           QUuid &shipToRemodel,
                           int factoryID);
QByteArray clientDemandDestructEquip(const QList<QUuid> &);
QByteArray clientDemandEquipInfo(QDateTime timeUtc
                                 = QDateTime(QDate(1970, 1, 1),
                                             QTime(0, 0, 0)));
QByteArray clientDemandEquipInfoUser();
QByteArray clientDemandMapInfo(QDateTime timeUtc
                               = QDateTime(QDate(1970, 1, 1),
                                           QTime(0, 0, 0)));
QByteArray clientDemandModernizeShip(const QList<QUuid> &);
QByteArray clientDemandResourceUpdate();
QByteArray clientDemandShipInfo(QDateTime timeUtc
                                = QDateTime(QDate(1970, 1, 1),
                                            QTime(0, 0, 0)));
QByteArray clientDemandShipInfoUser();
QByteArray clientDemandSkillPoints(int);
QByteArray clientDemandTech(int local = 0);
QByteArray clientDevelop(int, bool convert = false, int factoryID = -1);
QByteArray clientFactoryRefresh();
QByteArray clientFetch(int factoryID = -1);
QByteArray clientFleetData(const QJsonArray &);
QByteArray clientHello();
QByteArray clientHomePort(ShipNationality);
QByteArray clientMigrate(const QJsonObject &);
QByteArray clientStateChange(GameState);
QByteArray clientSteamAuth(uint8 [], uint32);
QByteArray clientSteamLogout();
QByteArray clientTestMessages(int);

QByteArray serverAskForHomePort();
QByteArray serverBlueprintRetired(int);
QByteArray serverDevelopFailed(GameError);
QByteArray serverDevelopStart(bool construct = false);
QByteArray serverEquipLackFather(GameError, int);
QByteArray serverEquipLackMother(GameError, int, int64);
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
QByteArray serverMapInfo(const QJsonArray &, bool user = false,
                         QDateTime timeUtc = QDateTime::currentDateTimeUtc(),
                         bool cacheHit = false);
QByteArray serverNewEquip(QUuid, int);
QByteArray serverNewShip(QUuid, int, int);
QByteArray serverParseError(MsgType, const QString &,
                            const QString &);
QByteArray serverPenguin();
QByteArray serverResourceUpdate(ResOrd);
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

};

#endif // KP_H
