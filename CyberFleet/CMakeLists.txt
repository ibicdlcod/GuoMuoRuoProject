cmake_minimum_required(VERSION 3.16)

project(CyberFleet VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets LinguistTools
    Sql Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui UiPlugin Widgets LinguistTools
    Sql Network)

include_directories(${root_SOURCE_DIR}/FactorySlot)
add_subdirectory(FactorySlot)

set(TS_FILES
    Translations/CyberFleet2_zh_CN.ts
    Translations/CyberFleet2_en_US.ts)

set(CLIENT_SOURCES
    ClientGUI/clientv2.cpp
    ClientGUI/clientv2.h
    ClientGUI/main.cpp
    ClientGUI/networkerror.cpp
    ClientGUI/networkerror.h
    ClientGUI/steamauth.cpp
    ClientGUI/steamauth.h
    ClientGUI/ui/developwindow.cpp
    ClientGUI/ui/developwindow.h
    ClientGUI/ui/developwindow.ui
    ClientGUI/ui/factoryarea.cpp
    ClientGUI/ui/factoryarea.h
    ClientGUI/ui/factoryarea.ui
    ClientGUI/ui/keyenterreceiver.cpp
    ClientGUI/ui/keyenterreceiver.h
    ClientGUI/ui/licensearea.cpp
    ClientGUI/ui/licensearea.h
    ClientGUI/ui/licensearea.ui
    ClientGUI/ui/mainwindow.cpp
    ClientGUI/ui/mainwindow.h
    ClientGUI/ui/mainwindow.ui
    ClientGUI/ui/newlogins.cpp
    ClientGUI/ui/newlogins.h
    ClientGUI/ui/newlogins.ui
    ClientGUI/ui/portarea.cpp
    ClientGUI/ui/portarea.h
    ClientGUI/ui/portarea.ui
    ${TS_FILES}
)
set(SERVER_SOURCES
    Server/kerrors.cpp
    Server/kerrors.h
    Server/main.cpp
    Server/peerinfo.cpp
    Server/peerinfo.h
    Server/server.cpp
    Server/server.h
    Server/sslserver.cpp
    Server/sslserver.h
    Server/tech.cpp
    Server/tech.h
    Server/user.cpp
    Server/user.h
    ${TS_FILES}
)
set(PROTOCOL_SOURCES
    Protocol/commandline.cpp
    Protocol/commandline.h
    Protocol/consoletextstream.cpp
    Protocol/consoletextstream.h
    Protocol/ecma48.cpp
    Protocol/ecma48.h
    Protocol/equipment.cpp
    Protocol/equipment.h
    Protocol/equiptype.cpp
    Protocol/equiptype.h
    Protocol/kp.cpp
    Protocol/kp.h
    Protocol/resord.cpp
    Protocol/resord.h
    Protocol/wcwidth.c
    Protocol/wcwidth.h
)

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    enable_language("RC")
    set(app_icon_resource_windows "${CMAKE_CURRENT_SOURCE_DIR}/resources/icon.rc")
endif()

qt_add_library(CFProtocol
    STATIC
    MANUAL_FINALIZATION
    ${PROTOCOL_SOURCES}
)
qt_add_executable(CFClient
    MANUAL_FINALIZATION
    ${CLIENT_SOURCES}
    common.qrc
    ${app_icon_resource_windows}
)
qt_add_executable(CFServer
    MANUAL_FINALIZATION
    ${SERVER_SOURCES}
    common.qrc
    Server/qconsolelistener.cpp Server/qconsolelistener.h
    Server/tech.cpp Server/tech.h

)
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    find_library(STEAM_LIBRARY steam_api64 HINTS redistributable_bin/win64)
else()
    find_library(STEAM_LIBRARY steam_api HINTS redistributable_bin/linux64)
endif()
if( ${STEAM_LIBRARY} STREQUAL "STEAM_LIBRARY-NOTFOUND")
    message("Please set STEAM_LIBRARY variable to location of redistributable_bin/(your arch)")
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    find_library(STEAM_APPENCRYPT_LIBRARY sdkencryptedappticket64 HINTS steam/lib/win64)
else()
    find_library(STEAM_APPENCRYPT_LIBRARY sdkencryptedappticket HINTS steam/lib/linux64)
endif()
if( ${STEAM_APPENCRYPT_LIBRARY} STREQUAL "STEAM_APPENCRYPT_LIBRARY-NOTFOUND")
    message("Please set STEAM_LIBRARY variable to location of steam/lib/(your arch)")
endif()

# Define target properties for Android with Qt 6 as:
#    set_property(TARGET CyberFleet2 APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation

#qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})

message("add tr")
qt6_add_translations(CFClient
    TS_FILES ${TS_FILES}
    INCLUDE_DIRECTORIES {root_SOURCE_DIR}
    LRELEASE_OPTIONS -idbased)
qt6_add_translations(CFServer
    TS_FILES ${TS_FILES}
    INCLUDE_DIRECTORIES {root_SOURCE_DIR}
    LRELEASE_OPTIONS -idbased)

target_link_libraries(CFProtocol PRIVATE Qt${QT_VERSION_MAJOR}::Core)
target_link_libraries(CFProtocol PRIVATE Qt${QT_VERSION_MAJOR}::Sql)
target_link_libraries(CFClient PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(CFClient PRIVATE Qt${QT_VERSION_MAJOR}::Network)
target_link_libraries(CFClient PRIVATE CFProtocol)
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    target_link_libraries(CFClient PRIVATE factoryslot/FactorySlot)
else ()
    target_link_libraries(CFClient PRIVATE ${CMAKE_BINARY_DIR}/FactorySlot/libFactorySlot.so)
endif()
target_link_libraries(CFClient PRIVATE ${STEAM_LIBRARY})
target_link_libraries(CFClient PRIVATE ${STEAM_APPENCRYPT_LIBRARY})
target_link_libraries(CFServer PRIVATE Qt${QT_VERSION_MAJOR}::Core)
target_link_libraries(CFServer PRIVATE Qt${QT_VERSION_MAJOR}::Sql)
target_link_libraries(CFServer PRIVATE Qt${QT_VERSION_MAJOR}::Network)
target_link_libraries(CFServer PRIVATE CFProtocol)
target_link_libraries(CFServer PRIVATE ${STEAM_LIBRARY})
target_link_libraries(CFServer PRIVATE ${STEAM_APPENCRYPT_LIBRARY})

set_target_properties(CFClient PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER cyberfleet.xyz
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

qt_finalize_target(CFProtocol)
if (${CMAKE_BUILD_TYPE} MATCHES "Release")
    target_compile_definitions(CFClient PUBLIC QT_DEBUG_NO_OUTPUT)
    target_compile_definitions(CFServer PUBLIC QT_DEBUG_NO_OUTPUT)
endif()
qt_finalize_executable(CFClient)
qt_finalize_executable(CFServer)
