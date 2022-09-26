#include "equipment.h"
#include <QRegularExpression>
#include <QVariant>
#include <QMetaEnum>

EquipType::EquipType(const QString &basis) {

    static QRegularExpression rehex(
                "^([a-zA-Z]+)(-[a-zA-Z]+(\\|[a-zA-Z]+)*)?(-\\d+)?$");
    auto m = rehex.match(basis);

    if(m.hasMatch()) {
        QString baseStr = m.captured(1);
        if(baseStr.isNull())
            base = Disabled;
        else {
            QMetaEnum info = QMetaEnum::fromType<BasicType>();
            base = BasicType(info.keyToValue(
                                 baseStr.toLatin1().constData()));
        }
        QString flagsStr = m.captured(2);
        if(flagsStr.isNull())
            flags = TypeFlags();
        else {
            flagsStr.remove(0, 1);
            QMetaEnum info = QMetaEnum::fromType<TypeFlags>();
            flags = TypeFlags(info.keysToValue(
                                  flagsStr.toLatin1().constData()));
            if(flags == -1)
                flags = TypeFlags();
        }
        QString sizeStr = m.captured(4);
        if(sizeStr.isNull())
            size = 0;
        else {
            sizeStr.remove(0, 1);
            size = sizeStr.toInt();
        }
    }
    else {
        base = Disabled;
        flags = TypeFlags();
        size = 0;
    }
}

QString EquipType::toString() const {
    QString baseStr = QVariant::fromValue(base).toString();
    QMetaEnum info = QMetaEnum::fromType<TypeFlags>();
    if(!flags.testFlags(TypeFlags())) {
        baseStr.append("-");
        baseStr.append(info.valueToKeys(flags));
    }
    if(size > 0) {
        baseStr.append("-");
        baseStr.append(QString::number(size));
    }
    return baseStr;
}

Equipment::Equipment(int id,
                     QString &&name,
                     QString &&type,
                     QMap<AttrType, int> &&attr,
                     QStringList &&customflags)
    : id(id),
      name(name),
      type(type),
      attr(attr),
      customflags(customflags) {
}
