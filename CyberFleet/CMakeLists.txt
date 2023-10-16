cmake_minimum_required(VERSION 3.16)

project(CyberFleet2 VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets LinguistTools
    Sql Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui UiPlugin Widgets LinguistTools
    Sql Network)

include_directories(${root_SOURCE_DIR}/factoryslot)
add_subdirectory(factoryslot)

set(TS_FILES
    translations/CyberFleet2_zh_CN.ts
    translations/CyberFleet2_en_US.ts)

set(CLIENT_SOURCES
    ClientGUI/clientv2.cpp
    ClientGUI/clientv2.h
    ClientGUI/main.cpp
    ClientGUI/networkerror.cpp
    ClientGUI/networkerror.h
    ClientGUI/steamauth.cpp
    ClientGUI/steamauth.h
    ${TS_FILES}
)
set(SERVER_SOURCES
    server/main.cpp
    server/kerrors.cpp
    server/kerrors.h
    server/qconsolelistener.cpp
    server/qconsolelistener.h
    server/server.cpp
    server/server.h
    server/sslserver.cpp
    server/sslserver.h
    server/user.cpp
    server/user.h
    ${TS_FILES}
)
set(PROTOCOL_SOURCES
    protocol/commandline.cpp
    protocol/commandline.h
    protocol/consoletextstream.cpp
    protocol/consoletextstream.h
    protocol/ecma48.cpp
    protocol/ecma48.h
    protocol/equipment.cpp
    protocol/equipment.h
    protocol/kp.cpp
    protocol/kp.h
    protocol/resord.cpp
    protocol/resord.h
    protocol/wcwidth.c
    protocol/wcwidth.h
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

# Define target properties for Android with Qt 6 as:
#    set_property(TARGET CyberFleet2 APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation

#qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})

qt6_add_translations(CFClient
    TS_FILES ${TS_FILES}
    LRELEASE_OPTIONS -idbased)
qt6_add_translations(CFServer
    TS_FILES ${TS_FILES}
    LRELEASE_OPTIONS -idbased)


target_link_libraries(CFProtocol PRIVATE Qt${QT_VERSION_MAJOR}::Core)
target_link_libraries(CFClient PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(CFClient PRIVATE Qt${QT_VERSION_MAJOR}::Network)
target_link_libraries(CFClient PRIVATE CFProtocol)
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    target_link_libraries(CFClient PRIVATE factoryslot/FactorySlot)
else ()
    target_link_libraries(CFClient PRIVATE ${CMAKE_BINARY_DIR}/factoryslot/libFactorySlot.so)
endif()
target_link_libraries(CFServer PRIVATE Qt${QT_VERSION_MAJOR}::Core)
target_link_libraries(CFServer PRIVATE Qt${QT_VERSION_MAJOR}::Sql)
target_link_libraries(CFServer PRIVATE Qt${QT_VERSION_MAJOR}::Network)
target_link_libraries(CFServer PRIVATE CFProtocol)

set_target_properties(CFClient PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER cyberfleet.xyz
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

qt_finalize_target(CFProtocol)
qt_finalize_executable(CFClient)
if (${CMAKE_BUILD_TYPE} MATCHES "Release")
    target_compile_definitions(CFClient PUBLIC QT_DEBUG_NO_OUTPUT)
    target_compile_definitions(CFServer PUBLIC QT_DEBUG_NO_OUTPUT)
endif()
qt_finalize_executable(CFServer)
