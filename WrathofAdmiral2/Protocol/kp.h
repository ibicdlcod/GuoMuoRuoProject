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
        UserExists
    };
    Q_ENUM_NS(AuthError)

    enum MsgType{
        JsonError,
        Unsupported,
        AccessDenied,
        DevelopFailed
    };
    Q_ENUM_NS(MsgType)

    enum GameState{
        Offline,
        Port,
        Factory
    };
    Q_ENUM_NS(GameState)

    enum ResourceType{
        ResOil,
        ResAmmo,
        ResMetal,
        ResRare
    };
    Q_ENUM_NS(ResourceType)

    enum CommandType{
        Develop
    };
    Q_ENUM_NS(CommandType)

    void initLog(bool server = false);
    void winConsoleCheck();

    /* See JSON support in Qt, especially QCborValue */
    QByteArray clientAuth(AuthMode, const QString &name = "",
                          const QByteArray &shadow = "");
    QByteArray serverAuth(AuthMode, const QString &,
                          bool, AuthError);
    QByteArray serverAuth(AuthMode, const QString &,
                          bool);
    QByteArray serverParseError(MsgType, const QString &,
                           const QString &);
    QByteArray clientDevelop(int, bool convert = false);
    QByteArray accessDenied();
    QByteArray serverDevelopFailed(bool ruleBased = true);
};

#endif // KP_H
