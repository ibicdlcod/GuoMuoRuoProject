QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    clientv2.cpp \
    ui/loginscreen.cpp \
    main.cpp \
    networkerror.cpp \
    steamauth.cpp \
    ui/developwindow.cpp \
    ui/keyenterreceiver.cpp \
    ui/licensearea.cpp \
    ui/mainwindow.cpp \
    ui/portarea.cpp

HEADERS += \
    clientv2.h \
    ui/loginscreen.h \
    networkerror.h \
    steamauth.h \
    ui/developwindow.h \
    ui/keyenterreceiver.h \
    ui/licensearea.h \
    ui/mainwindow.h \
    ui/portarea.h

FORMS += \
    ui/loginscreen.ui \
    ui/developwindow.ui \
    ui/licensearea.ui \
    ui/mainwindow.ui \
    ui/portarea.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    ../common.qrc

CONFIG += lrelease
CONFIG += embed_translations
QMAKE_LRELEASE_FLAGS += -idbased

TRANSLATIONS += \
    ../Translations/WA2_en_US.ts \
    ../Translations/WA2_zh_CN.ts

win32:RC_ICONS += ../resources/icon.ico

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../Protocol/release/ -lProtocol
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../Protocol/debug/ -lProtocol
else:unix: LIBS += -L$$OUT_PWD/../Protocol/ -lProtocol

INCLUDEPATH += $$PWD/../Protocol
DEPENDPATH += $$PWD/../Protocol

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Protocol/release/libProtocol.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Protocol/debug/libProtocol.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Protocol/release/Protocol.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Protocol/debug/Protocol.lib
else:unix: PRE_TARGETDEPS += $$OUT_PWD/../Protocol/libProtocol.a

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../build-CyberFleet-Desktop_Qt_6_6_0_MSVC2019_64bit-Debug/FactorySlot/debug/ -lFactorySlotplugin
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../build-CyberFleet-Desktop_Qt_6_6_0_MSVC2019_64bit-Debug/FactorySlot/debug/ -lFactorySlotplugind
else:unix: LIBS += -L$$PWD/../../build-CyberFleet-Desktop_Qt_6_6_0_MSVC2019_64bit-Debug/FactorySlot/debug/ -lFactorySlotplugind

INCLUDEPATH += $$PWD/../FactorySlot
DEPENDPATH += $$PWD/../FactorySlot

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../../build-CyberFleet-Desktop_Qt_6_6_0_MSVC2019_64bit-Debug/FactorySlot/debug/libFactorySlotplugin.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../../build-CyberFleet-Desktop_Qt_6_6_0_MSVC2019_64bit-Debug/FactorySlot/debug/libFactorySlotplugind.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../../build-CyberFleet-Desktop_Qt_6_6_0_MSVC2019_64bit-Debug/FactorySlot/debug/FactorySlotplugin.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../../build-CyberFleet-Desktop_Qt_6_6_0_MSVC2019_64bit-Debug/FactorySlot/debug/FactorySlotplugind.lib
else:unix: PRE_TARGETDEPS += $$PWD/../../build-CyberFleet-Desktop_Qt_6_6_0_MSVC2019_64bit-Debug/FactorySlot/debug/libFactorySlotplugind.a

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../redistributable_bin/win64/ -lsteam_api64
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../redistributable_bin/win64/ -lsteam_api64

INCLUDEPATH += $$PWD/../redistributable_bin
DEPENDPATH += $$PWD/../redistributable_bin
