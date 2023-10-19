#ifndef KP_H
#define KP_H

#define STRING2(x) #x
#define STRING(x) STRING2(x)
#define M_CONST __FILE__ STRING(:__LINE__: MAGICCONSTANT UNDESIREABLE NO 1)
#define NOT_M_CONST __FILE__ STRING(:__LINE__: This is considered an integral part of the program rather than magic constants.)
#define SALT_FISH __FILE__ STRING(:__LINE__: This is a salt fish.)
#define USED_CXX17 __FILE__ STRING(:__LINE__: This part uses C++ 17 features. Use macro "__cplusplus" to check whether your compiler supports it.)

#include <QObject>
#include <QJsonObject>
#include <QCborValue>
#include "../steam/isteamuser.h"

/* OS Specific */
#if defined (Q_OS_WIN)
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#elif defined (Q_OS_UNIX)
#include <locale.h>
#endif

namespace KP {
Q_NAMESPACE

#pragma message(NOT_M_CONST)
static constexpr int initDock = 4;
static constexpr int initFactory = 4;
static constexpr qint64 secsinMin = 60;
static constexpr float baseDevRarity = 8.0;
static constexpr int equipIdMax = 0x10000;

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
    ResourceRequired,
    FairyBusy,
    Penguin,
    NewEquip,
    Hello,
    LackPrivate,
    AllowClientStart,
    AllowClientFinish,
    VerifyComplete
};
Q_ENUM_NS(MsgType)

enum GameState{
    Offline,
    Port,
    Factory,
    TechView
};
Q_ENUM_NS(GameState)

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
    AdminAddEquip,
    DemandEquipInfo
};
Q_ENUM_NS(CommandType)

enum GameError{
    ResourceLack,
    DevelopNotOption,
    DevelopNotExist,
    FactoryBusy
};
Q_ENUM_NS(GameError)

enum FactoryState{
    Development,
    Deprecation,
    ConvertDevelop,
    IndustrialPlant
};
Q_ENUM_NS(FactoryState)

enum InfoType{
    FactoryInfo,
    EquipInfo
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

void initLog(bool server = false);
#if defined (Q_OS_WIN)
void winConsoleCheck();
#endif

/* See JSON support in Qt, especially QCborValue */
QByteArray accessDenied();
QByteArray catbomb();
QByteArray clientAddEquip(int);
QByteArray clientDemandEquipInfo();
QByteArray clientDevelop(int, bool convert = false, int factoryID = -1);
QByteArray clientFactoryRefresh();
QByteArray clientFetch(int factoryID = -1);
QByteArray clientHello();
QByteArray clientStateChange(GameState);
QByteArray clientSteamAuth(uint8 [], uint32);
QByteArray clientSteamLogout();
QByteArray serverDevelopFailed(GameError);
QByteArray serverEquipLackFather(GameError, int);
QByteArray serverDevelopStart();
QByteArray serverEquipInfo(QJsonArray &);
QByteArray serverFairyBusy(int);
QByteArray serverHello();
QByteArray serverLackPrivate();
QByteArray serverLogFail(AuthFailType);
QByteArray serverLogSuccess(bool);
QByteArray serverLogout(LogoutType);
QByteArray serverNewEquip(int, int);
QByteArray serverParseError(MsgType, const QString &,
                            const QString &);
QByteArray serverPenguin();
QByteArray serverVerifyComplete();
QByteArray weighAnchor();
};

#endif // KP_H
