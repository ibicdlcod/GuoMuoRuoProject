# QT -= gui
QT += network core

CONFIG += c++11 console
CONFIG -= app_bundle

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        consoletextstream.cpp \
        dtlsserver.cpp \
        ecma48.cpp \
        main.cpp \
        qprint.cpp \
        run.cpp \
        wcwidth.cpp

TRANSLATIONS += \
    STServer_zh_CN.ts
CONFIG += lrelease
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    consoletextstream.h \
    dtlsserver.h \
    ecma48.h \
    qprint.h \
    run.h \
    wcwidth.h
