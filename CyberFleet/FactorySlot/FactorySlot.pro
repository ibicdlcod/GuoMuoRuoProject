CONFIG      += plugin debug_and_release
TARGET      = $$qtLibraryTarget(FactorySlotplugin)
TEMPLATE    = lib

RESOURCES   = icons.qrc
LIBS        += -L.

greaterThan(QT_MAJOR_VERSION, 4) {
    QT += designer
} else {
    CONFIG += designer
}

target.path = $$[QT_INSTALL_PLUGINS]/designer
INSTALLS    += target

include(FactorySlot.pri)

DISTFILES += \
    factoryslot.pri

HEADERS += \
    factoryslot.h \
    factoryslotplugin.h

SOURCES += \
    factoryslot.cpp \
    factoryslotplugin.cpp
