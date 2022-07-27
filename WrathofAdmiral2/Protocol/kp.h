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

    enum dgramType{
        auth
    };
    Q_ENUM(dgramType)

    enum authMode{
        login,
        reg,
        logout
    };
    Q_ENUM(authMode)

    /* See JSON support in Qt, especially QCborValue */
    static QByteArray clientAuth(authMode, const QString &name = "",
                                 const QByteArray &shadow = "");
    static QByteArray serverAuth();
};

#endif // KP_H
