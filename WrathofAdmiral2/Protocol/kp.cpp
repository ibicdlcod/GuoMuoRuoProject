#include "kp.h"

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

QByteArray KP::serverParseError(MsgType pe, const QString &uname,
                                const QString &content)
{
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = pe;
    result["username"] = uname;
    result["content"] = content;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::clientDevelop(int equipid, bool convert)
{
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::Develop;
    result["equipid"] = equipid;
    result["convert"] = convert;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::accessDenied()
{
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = MsgType::AccessDenied;
    return QCborValue::fromJsonValue(result).toCbor();
}

QByteArray KP::serverDevelopFailed(bool ruleBased)
{
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = MsgType::DevelopFailed;
    result["rulebased"] = ruleBased;
    return QCborValue::fromJsonValue(result).toCbor();
}
