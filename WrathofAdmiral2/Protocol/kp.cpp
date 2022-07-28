#include "kp.h"

/*
Protocol::Protocol()
{
}
*/

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

QByteArray KP::serverParse(ParseError p, const QString &uname, const QString &content)
{
    QJsonObject result;
    result["type"] = DgramType::Message;
    result["msgtype"] = p;
    result["username"] = uname;
    result["content"] = content;
    return QCborValue::fromJsonValue(result).toCbor();
}
//QCborValue::fromJsonValue(gameObject).toCbor();
//QCborValue::fromCbor(saveData).toMap().toJsonObject())

/* use Q_ENUM(); class ee enum A{B,C,D} Q_ENUM(A)
//QMetaEnum metaEnum = QMetaEnum::fromType<ee::A>();
//qDebug() << metaEnum.valueToKey(ee::B);
QVariant a; // do not use QVariant(ee:B)
a.setValue(ee::B);
std::cout << a.toString().toStdString() << std::endl;
qDebug() << ee::B;
*/
