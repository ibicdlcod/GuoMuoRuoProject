#include "kp.h"

#include "gamestate.h"

QByteArray KP::clientAuth(AuthMode mode, const QString &uname,
                          const QByteArray &shadow)
{
    QJsonObject result;
    result["type"] = DgramType::Auth;
    result["mode"] = mode;
    if(mode != AuthMode::Logout)
    {
        result["username"] = uname;
        /* directly using QString is even less efficient */
        result["shadow"] = QString(shadow.toBase64(QByteArray::Base64Encoding));
    }
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverAuth(AuthMode mode, const QString &uname,
                          bool success, AuthError reason)
{
    QJsonObject result;
    result["type"] = DgramType::Auth;
    result["mode"] = mode;
    result["username"] = uname;
    result["success"] = success;
    result["reason"] = reason;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverAuth(AuthMode mode, const QString &uname,
                          bool success)
{
    QJsonObject result;
    result["type"] = DgramType::Auth;
    result["mode"] = mode;
    result["username"] = uname;
    result["success"] = success;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverParse(MsgType pe, const QString &uname,
                           const QString &content)
{
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = pe;
    result["username"] = uname;
    result["content"] = content;
    return QCborValue::fromJsonValue(result).toCbor();
}
