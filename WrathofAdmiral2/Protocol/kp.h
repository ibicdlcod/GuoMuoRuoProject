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

class KP : public QObject
{
    Q_OBJECT

public:
    KP() = delete;

    enum DgramType{
        Auth,
        Message,
        Request
    };
    Q_ENUM(DgramType)

    enum AuthMode{
        Login,
        Reg,
        Logout
    };
    Q_ENUM(AuthMode)

    enum AuthError{
        BadShadow,
        BadPassword,
        LoggedElsewhere,
        UserExists
    };
    Q_ENUM(AuthError)

    enum MsgType{
        JsonError,
        Unsupported,
        AccessDenied,
        DevelopFailed
    };
    Q_ENUM(MsgType)

    enum GameState{
        Offline,
        Port,
        Factory
    };
    Q_ENUM(GameState)

    enum ResourceType{
        ResOil,
        ResAmmo,
        ResMetal,
        ResRare
    };
    Q_ENUM(ResourceType)

    enum CommandType{
        Develop
    };
    Q_ENUM(CommandType)

    /* See JSON support in Qt, especially QCborValue */
    static QByteArray clientAuth(AuthMode, const QString &name = "",
                                 const QByteArray &shadow = "");
    static QByteArray serverAuth(AuthMode, const QString &,
                                 bool, AuthError);
    static QByteArray serverAuth(AuthMode, const QString &,
                                 bool);
    static QByteArray serverParseError(MsgType, const QString &,
                                  const QString &);
    static QByteArray clientDevelop(int, bool convert = false);
    static QByteArray accessDenied();
    static QByteArray serverDevelopFailed(bool ruleBased = true);
};

#endif // KP_H
