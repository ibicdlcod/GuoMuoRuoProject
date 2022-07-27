#include "kp.h"

/*
Protocol::Protocol()
{
}
*/

QByteArray KP::clientAuth(authMode mode, const QString &uname,
                          const QByteArray &shadow)
{
    qDebug() << mode;
    QJsonObject result;
    result["type"] = dgramType::auth;
    result["mode"] = mode;
    if(mode != authMode::logout)
    {
        result["username"] = uname;
        /* directly using QString is even less efficient */
        result["shadow"] = QString(shadow.toBase64(QByteArray::Base64Encoding));
    }
    return QCborValue::fromJsonValue(result).toCbor();
}
/* Client:
 * type = auth
 * mode = login/reg/logout
 * shadow = (data)
 *
 * Server:
 * type = auth
 * success = (bool)
 * mode = login/reg/logout
 * reason = nopassword/incorrect
 *
 */

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
