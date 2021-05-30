#QT -= gui
QT += core

CONFIG += c++11 console
CONFIG -= app_bundle

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        console/qprint.cpp \
        console/runner.cpp \
        logic/stcommand.cpp \
        logic/stengine.cpp \
        logic/strelations.cpp \
        main.cpp

TRANSLATIONS += \
    test_zh_CN.ts
CONFIG += lrelease
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    console/qprint.h \
    console/runner.h \
    enum.h \
    logic/stcommand.h \
    logic/stengine.h \
    logic/strelations.h

DISTFILES += \
    openingwords.txt
