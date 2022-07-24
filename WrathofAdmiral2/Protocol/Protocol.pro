QT -= gui

TEMPLATE = lib
CONFIG += staticlib

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    commandline.cpp \
    consoletextstream.cpp \
    ecma48.cpp \
    protocol.cpp \
    wcwidth.cpp

HEADERS += \
    commandline.h \
    consoletextstream.h \
    ecma48.h \
    protocol.h \
    wcwidth.h

# Default rules for deployment.
unix {
    target.path = $$[QT_INSTALL_PLUGINS]/generic
}
!isEmpty(target.path): INSTALLS += target
