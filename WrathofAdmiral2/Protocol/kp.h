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
static constexpr int equipIdMax = 0x100000;

enum DgramType{
    Auth,
    Message,
    Request
};
Q_ENUM_NS(DgramType)

enum AuthMode{
    Login,
    Reg,
    Logout
};
Q_ENUM_NS(AuthMode)

enum AuthError{
    BadShadow,
    BadPassword,
    LoggedElsewhere,
    UserNonexist,
    UserExists,
    RetryToomuch
};
Q_ENUM_NS(AuthError)

enum MsgType{
    JsonError,
    Unsupported,
    AccessDenied,
    DevelopFailed,
    DevelopStart,
    ResourceRequired,
    FairyBusy,
    Penguin,
    NewEquip
};
Q_ENUM_NS(MsgType)

enum GameState{
    Offline,
    Port,
    Factory
};
Q_ENUM_NS(GameState)

enum ResourceType{
    Oil,
    Explosives,
    Steel,
    Rubber,
    Aluminium,
    Tungsten,
    Chromium
};
Q_ENUM_NS(ResourceType)

enum CommandType{
    ChangeState,
    Develop,
    Fetch
};
Q_ENUM_NS(CommandType)

enum GameError{
    ResourceLack,
    DevelopNotOption,
    FactoryBusy
};
Q_ENUM_NS(GameError)

void initLog(bool server = false);
#if defined (Q_OS_WIN)
void winConsoleCheck();
#endif

/* See JSON support in Qt, especially QCborValue */
QByteArray accessDenied();
QByteArray clientAuth(AuthMode, const QString &name = "",
                      const QByteArray &shadow = "");
QByteArray clientDevelop(int, bool convert = false, int factoryID = -1);
QByteArray clientFetch(int factoryID = -1);
QByteArray clientStateChange(GameState);
QByteArray serverAuth(AuthMode, const QString &,
                      bool, AuthError);
QByteArray serverAuth(AuthMode, const QString &,
                      bool, AuthError, QDateTime);
QByteArray serverAuth(AuthMode, const QString &,
                      bool);
QByteArray serverDevelopFailed(GameError);
QByteArray serverDevelopStart();
QByteArray serverFairyBusy(int);
QByteArray serverNewEquip(int, int);
QByteArray serverParseError(MsgType, const QString &,
                            const QString &);
QByteArray serverPenguin();
};

#endif // KP_H
