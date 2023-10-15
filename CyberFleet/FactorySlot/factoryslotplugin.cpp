// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "factoryslot.h"
#include "factoryslotplugin.h"

#include <QtPlugin>

FactorySlotPlugin::FactorySlotPlugin(QObject *parent)
    : QObject(parent)
{
}

void FactorySlotPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
    if (initialized)
        return;

    initialized = true;
}

bool FactorySlotPlugin::isInitialized() const
{
    return initialized;
}

QWidget *FactorySlotPlugin::createWidget(QWidget *parent)
{
    return new FactorySlot(parent);
}

QString FactorySlotPlugin::name() const
{
    return QStringLiteral("FactorySlot");
}

QString FactorySlotPlugin::group() const
{
    return QStringLiteral("Custom");
}

QIcon FactorySlotPlugin::icon() const
{
    return QIcon();
}

QString FactorySlotPlugin::toolTip() const
{
    return QString();
}

QString FactorySlotPlugin::whatsThis() const
{
    return QString();
}

bool FactorySlotPlugin::isContainer() const
{
    return false;
}

QString FactorySlotPlugin::domXml() const
{
    return QLatin1String(R"(
<ui language="c++">]
    <widget class="FactorySlot" name="Factory 0">
        <property name="text">
            <string>Factory 0</string>
        </property>
    </widget>
</ui>
)");
}

QString FactorySlotPlugin::includeFile() const
{
    return QStringLiteral("factoryslot.h");
}
