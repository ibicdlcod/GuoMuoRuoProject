cmake_minimum_required(VERSION 3.16)

project(CyberFleet VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS
    UiPlugin LinguistTools Sql SvgWidgets Network HttpServer)

include_directories(${root_SOURCE_DIR}/FactorySlot)
add_subdirectory(FactorySlot)

set(TS_FILES
    Translations/CyberFleet2_en_US.ts
    Translations/CyberFleet2_ja_JP.ts
    Translations/CyberFleet2_zh_CN.ts)

set(CLIENT_SOURCES
    ClientGUI/clientv2.cpp
    ClientGUI/clientv2.h
    ClientGUI/equipicon.h
    ClientGUI/equipicon.cpp
    ClientGUI/equipmodel.h
    ClientGUI/equipmodel.cpp
    ClientGUI/main.cpp
    ClientGUI/networkerror.cpp
    ClientGUI/networkerror.h
    ClientGUI/steamauth.cpp
    ClientGUI/steamauth.h
    ClientGUI/ui/boxcenterfusionstyle.h
    ClientGUI/ui/developwindow.cpp
    ClientGUI/ui/developwindow.h
    ClientGUI/ui/developwindow.ui
    ClientGUI/ui/equipview.h
    ClientGUI/ui/equipview.cpp
    ClientGUI/ui/equipview.ui
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
    ClientGUI/ui/maprender.cpp
    ClientGUI/ui/maprender.h
    ClientGUI/ui/mapviewwidget.cpp
    ClientGUI/ui/mapviewwidget.h
    ClientGUI/ui/navigator.cpp
    ClientGUI/ui/navigator.h
    ClientGUI/ui/newlogins.cpp
    ClientGUI/ui/newlogins.h
    ClientGUI/ui/newlogins.ui
    ClientGUI/ui/portarea.cpp
    ClientGUI/ui/portarea.h
    ClientGUI/ui/portarea.ui
    ClientGUI/ui/sortie.cpp
    ClientGUI/ui/sortie.h
    ClientGUI/ui/sortie.ui
    ClientGUI/ui/techview.h
    ClientGUI/ui/techview.cpp
    ClientGUI/ui/techview.ui
    ${TS_FILES}
)
set(SERVER_SOURCES
    Server/kerrors.cpp
    Server/kerrors.h
    Server/main.cpp
    Server/qconsolelistener.cpp
    Server/qconsolelistener.h
    Server/server.cpp
    Server/server.h
    Server/servermastersender.cpp
    Server/servermastersender.h
    Server/sslserver.cpp
    Server/sslserver.h
    Server/user.cpp
    Server/user.h
    Server/erfinv.h
    Server/rngesus.h
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
    Protocol/map.cpp
    Protocol/map.h
    Protocol/mapwithdiff.cpp
    Protocol/mapwithdiff.h
    Protocol/peerinfo.cpp
    Protocol/peerinfo.h
    Protocol/receiver.cpp
    Protocol/receiver.h
    Protocol/resord.cpp
    Protocol/resord.h
    Protocol/sender.cpp
    Protocol/sender.h
    Protocol/ship.cpp
    Protocol/ship.h
    Protocol/shiptype.cpp
    Protocol/shiptype.h
    Protocol/tech.cpp
    Protocol/tech.h
    Protocol/utility.cpp
    Protocol/utility.h
    Protocol/wcwidth.c
    Protocol/wcwidth.h
    ${TS_FILES}
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

qt_add_translations(
    TARGETS CFClient CFProtocol CFServer
    TS_FILES ${TS_FILES}
    INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}
    LRELEASE_OPTIONS -idbased)

# frequently error but not always
add_dependencies(CFProtocol update_translations)
add_dependencies(CFClient update_translations)
add_dependencies(CFServer update_translations)

target_link_libraries(CFProtocol PRIVATE Qt${QT_VERSION_MAJOR}::Core)
target_link_libraries(CFProtocol PRIVATE Qt${QT_VERSION_MAJOR}::Sql)
target_link_libraries(CFProtocol PRIVATE Qt${QT_VERSION_MAJOR}::Network)
target_link_libraries(CFClient PRIVATE Qt${QT_VERSION_MAJOR}::HttpServer)
target_link_libraries(CFClient PRIVATE Qt${QT_VERSION_MAJOR}::SvgWidgets)
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
    MACOSX_BUNDLE_GUI_IDENTIFIER xyz.harusoft.cyberfleet
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

message(PROJECT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR})
message(PROJECT_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR})

# you still need to manually copy serverprivate.key and AppSecretKey
# to build directory
add_custom_command(TARGET CFClient POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/steam_appid.txt"
    $<TARGET_FILE_DIR:CFClient>)
add_custom_command(TARGET CFServer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/steam_appid.txt"
    $<TARGET_FILE_DIR:CFServer>)

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    add_custom_command(TARGET CFClient POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_BINARY_DIR}/FactorySlot/FactorySlot.dll"
        $<TARGET_FILE_DIR:CFClient>)
    add_custom_command(TARGET CFServer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_BINARY_DIR}/FactorySlot/FactorySlot.dll"
        $<TARGET_FILE_DIR:CFServer>)
    add_custom_command(TARGET CFClient POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/redistributable_bin/win64/steam_api64.dll"
        $<TARGET_FILE_DIR:CFClient>)
    add_custom_command(TARGET CFServer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/redistributable_bin/win64/steam_api64.dll"
        $<TARGET_FILE_DIR:CFServer>)
    add_custom_command(TARGET CFServer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/steam/lib/win64/sdkencryptedappticket64.dll"
        $<TARGET_FILE_DIR:CFServer>)
else ()
    add_custom_command(TARGET CFClient POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_BINARY_DIR}/FactorySlot/libFactorySlot.so"
        $<TARGET_FILE_DIR:CFClient>)
    add_custom_command(TARGET CFServer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_BINARY_DIR}/FactorySlot/libFactorySlot.so"
        $<TARGET_FILE_DIR:CFServer>)
    add_custom_command(TARGET CFClient POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/redistributable_bin/linux64/libsteam_api.so"
        $<TARGET_FILE_DIR:CFClient>)
    add_custom_command(TARGET CFServer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/redistributable_bin/linux64/libsteam_api.so"
        $<TARGET_FILE_DIR:CFServer>)
    add_custom_command(TARGET CFServer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/steam/lib/linux64/libsdkencryptedappticket.so"
        $<TARGET_FILE_DIR:CFServer>)
endif()
