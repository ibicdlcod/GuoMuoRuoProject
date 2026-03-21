// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "repairslot.h"
#include "repairslotplugin.h"

#include <QtPlugin>

RepairSlotPlugin::RepairSlotPlugin(QObject *parent)
    : QObject(parent)
{
}

void RepairSlotPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
    if (initialized)
        return;

    initialized = true;
}

bool RepairSlotPlugin::isInitialized() const
{
    return initialized;
}

QWidget *RepairSlotPlugin::createWidget(QWidget *parent)
{
    return new RepairSlot(parent);
}

QString RepairSlotPlugin::name() const
{
    return QStringLiteral("RepairSlot");
}

QString RepairSlotPlugin::group() const
{
    return QStringLiteral("Custom");
}

QIcon RepairSlotPlugin::icon() const
{
    return QIcon();
}

QString RepairSlotPlugin::toolTip() const
{
    return QString();
}

QString RepairSlotPlugin::whatsThis() const
{
    return QString();
}

bool RepairSlotPlugin::isContainer() const
{
    return false;
}

QString RepairSlotPlugin::domXml() const
{
    return QLatin1String(R"(
<ui language="c++">]
    <widget class="RepairSlot" name="Repair 0">
        <property name="text">
            <string>Repair 0</string>
        </property>
    </widget>
</ui>
)");
}

QString RepairSlotPlugin::includeFile() const
{
    return QStringLiteral("repairslot.h");
}
