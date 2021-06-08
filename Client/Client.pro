QT -= gui
QT += network

CONFIG += c++11 console
CONFIG -= app_bundle

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        dtlsclient.cpp \
        main.cpp

TRANSLATIONS += \
    Client_zh_CN.ts
CONFIG += lrelease
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

#include($$PWD/../QConsoleListener/src/qconsolelistener.pri)

HEADERS += \
    dtlsclient.h

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../QConsoleListener/release/ -lQConsoleListener
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../QConsoleListener/debug/ -lQConsoleListener
else:unix: LIBS += -L$$OUT_PWD/../QConsoleListener/ -lQConsoleListener

INCLUDEPATH += $$PWD/../QConsoleListener
DEPENDPATH += $$PWD/../QConsoleListener

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../QConsoleListener/release/libQConsoleListener.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../QConsoleListener/debug/libQConsoleListener.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../QConsoleListener/release/QConsoleListener.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../QConsoleListener/debug/QConsoleListener.lib
else:unix: PRE_TARGETDEPS += $$OUT_PWD/../QConsoleListener/libQConsoleListener.a

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../Magic/release/ -lMagic
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../Magic/debug/ -lMagic
else:unix: LIBS += -L$$OUT_PWD/../Magic/ -lMagic

INCLUDEPATH += $$PWD/../Magic
DEPENDPATH += $$PWD/../Magic
