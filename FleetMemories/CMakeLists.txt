# Copyright (C) 2026 Harusoft Ltd.
# SPDX-License-Identifier: AGPL-3.0-or-later

cmake_minimum_required(VERSION 3.16)

project(FleetMemories VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(CMAKE_CXX_STANDARD 20)
else ()
    set(CMAKE_CXX_STANDARD 23)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Lua
add_subdirectory(lua-cmake/)
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    if (${CMAKE_BUILD_TYPE} MATCHES "Release")
        set(LUA_LIBRARY ${PROJECT_BINARY_DIR}/lua-cmake/src/lua-5.4.lib)
    else ()
        set(LUA_LIBRARY ${PROJECT_BINARY_DIR}/lua-cmake/src/lua-5.4d.lib)
    endif()
else ()
    set(LUA_LIBRARY ${PROJECT_BINARY_DIR}/lua-cmake/src/liblua-5.4.a)
endif()
list(APPEND CMAKE_PREFIX_PATH ${PROJECT_SOURCE_DIR}/lua-cmake/src)
include_directories(${PROJECT_SOURCE_DIR}/lua-cmake/src)
include_directories(${PROJECT_BINARY_DIR}/lua-cmake/src)
find_package(Lua REQUIRED)

include_directories(${PROJECT_SOURCE_DIR}/sol2/include)
# end Lua

# OpenGL
set(TINYGLTF_HEADER_ONLY ON CACHE INTERNAL "" FORCE)
set(TINYGLTF_INSTALL OFF CACHE INTERNAL "" FORCE)
add_subdirectory(${PROJECT_SOURCE_DIR}/ClientGUI/ui/sortie/opengl/tinygltf)
# end OpenGL

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS
    UiPlugin LinguistTools Sql SvgWidgets Network HttpServer OpenGL OpenGLWidgets)

include_directories(${root_SOURCE_DIR}/FactorySlot)
include_directories(${root_SOURCE_DIR}/RepairSlot)

add_subdirectory(FactorySlot)
add_subdirectory(RepairSlot)

set(TS_FILES
    Translations/FleetMemories_en_US.ts
    Translations/FleetMemories_ja_JP.ts
    Translations/FleetMemories_zh_CN.ts)

set(CLIENT_SOURCES
    ClientGUI/clientv2.cpp
    ClientGUI/clientv2.h
    ClientGUI/clientv2_actions.cpp
    ClientGUI/clientv2_cache.cpp
    ClientGUI/clientv2_command.cpp
    ClientGUI/equipicon.cpp
    ClientGUI/equipicon.h
    ClientGUI/main.cpp
    ClientGUI/model/equipmodel.cpp
    ClientGUI/model/equipmodel.h
    ClientGUI/model/localeawaresort.cpp
    ClientGUI/model/localeawaresort.h
    ClientGUI/model/rankmodel.cpp
    ClientGUI/model/rankmodel.h
    ClientGUI/model/shipbpmodel.cpp
    ClientGUI/model/shipbpmodel.h
    ClientGUI/model/shipdefmodel.cpp
    ClientGUI/model/shipdefmodel.h
    ClientGUI/model/shipmodel.cpp
    ClientGUI/model/shipmodel.h
    ClientGUI/model/specequipmodel.cpp
    ClientGUI/model/specequipmodel.h
    ClientGUI/model/specshipmodel.cpp
    ClientGUI/model/specshipmodel.h
    ClientGUI/networkerror.cpp
    ClientGUI/networkerror.h
    ClientGUI/resourcefetch.cpp
    ClientGUI/resourcefetch.h
    ClientGUI/steamauth.cpp
    ClientGUI/steamauth.h
    ClientGUI/ui/boxcenterfusionstyle.h
    ClientGUI/ui/factory/constructwindow.cpp
    ClientGUI/ui/factory/constructwindow.h
    ClientGUI/ui/factory/constructwindow.ui
    ClientGUI/ui/factory/developwindow.cpp
    ClientGUI/ui/factory/developwindow.h
    ClientGUI/ui/factory/developwindow.ui
    ClientGUI/ui/factory/factoryarea.cpp
    ClientGUI/ui/factory/factoryarea.h
    ClientGUI/ui/factory/factoryarea.ui
    ClientGUI/ui/fleet/elidedlabel.cpp
    ClientGUI/ui/fleet/elidedlabel.h
    ClientGUI/ui/fleet/fleetview.cpp
    ClientGUI/ui/fleet/fleetview.h
    ClientGUI/ui/fleet/fleetview.ui
    ClientGUI/ui/fleet/interactivelabel.cpp
    ClientGUI/ui/fleet/interactivelabel.h
    ClientGUI/ui/fleet/segmentedhpbar.cpp
    ClientGUI/ui/fleet/segmentedhpbar.h
    ClientGUI/ui/fleet/battleresultshipdisplay.cpp
    ClientGUI/ui/fleet/battleresultshipdisplay.h
    ClientGUI/ui/fleet/shipdisplay.cpp
    ClientGUI/ui/fleet/shipdisplay.h
    ClientGUI/ui/fleet/shipdisplay.ui
    ClientGUI/ui/fleet/shipdisplayflat.cpp
    ClientGUI/ui/fleet/shipdisplayflat.h
    ClientGUI/ui/fleet/shipdisplayflat.ui
    ClientGUI/ui/fleet/shipattrdialog.cpp
    ClientGUI/ui/fleet/shipattrdialog.h
    ClientGUI/ui/fleet/shipequip.cpp
    ClientGUI/ui/fleet/shipequip.h
    ClientGUI/ui/fleet/shipequip.ui
    ClientGUI/ui/keyenterreceiver.cpp
    ClientGUI/ui/keyenterreceiver.h
    ClientGUI/ui/maintenance/repair.cpp
    ClientGUI/ui/maintenance/repair.h
    ClientGUI/ui/maintenance/repair.ui
    ClientGUI/ui/mainwindow.cpp
    ClientGUI/ui/mainwindow.h
    ClientGUI/ui/mainwindow.ui
    ClientGUI/ui/navigator.cpp
    ClientGUI/ui/navigator.h
    ClientGUI/ui/port/choosehomeport.cpp
    ClientGUI/ui/port/choosehomeport.h
    ClientGUI/ui/port/choosehomeport.ui
    ClientGUI/ui/port/licensearea.cpp
    ClientGUI/ui/port/licensearea.h
    ClientGUI/ui/port/licensearea.ui
    ClientGUI/ui/port/newlogins.cpp
    ClientGUI/ui/port/newlogins.h
    ClientGUI/ui/port/newlogins.ui
    ClientGUI/ui/port/portarea.cpp
    ClientGUI/ui/port/portarea.h
    ClientGUI/ui/port/portarea.ui
    ClientGUI/ui/settingswindow.cpp
    ClientGUI/ui/settingswindow.h
    ClientGUI/ui/settingswindow.ui
    ClientGUI/ui/shop/ardcoupondialog.cpp
    ClientGUI/ui/shop/ardcoupondialog.h
    ClientGUI/ui/shop/buyequipdialog.cpp
    ClientGUI/ui/shop/buyequipdialog.h
    ClientGUI/ui/shop/buyordresourcesdialog.cpp
    ClientGUI/ui/shop/buyordresourcesdialog.h
    ClientGUI/ui/shop/medalbuydialog.cpp
    ClientGUI/ui/shop/medalbuydialog.h
    ClientGUI/ui/sortie/battleplan.cpp
    ClientGUI/ui/sortie/battleplan.h
    ClientGUI/ui/sortie/battleplan.ui

    ClientGUI/ui/sortie/confirmsortie.cpp
    ClientGUI/ui/sortie/confirmsortie.h
    ClientGUI/ui/sortie/confirmsortie.ui
    ClientGUI/ui/sortie/mapdetail.cpp
    ClientGUI/ui/sortie/mapdetail.h
    ClientGUI/ui/sortie/mapdetail.ui
    ClientGUI/ui/sortie/maprender.cpp
    ClientGUI/ui/sortie/maprender.h
    ClientGUI/ui/sortie/mapviewwidget.cpp
    ClientGUI/ui/sortie/mapviewwidget.h
    ClientGUI/ui/sortie/resourcegainview.cpp
    ClientGUI/ui/sortie/resourcegainview.h
    ClientGUI/ui/sortie/opengl/battlewidget.cpp
    ClientGUI/ui/sortie/opengl/battlewidget.h
    ClientGUI/ui/sortie/opengl/geometryengine.cpp
    ClientGUI/ui/sortie/opengl/geometryengine.h
    ClientGUI/ui/sortie/sortie.cpp
    ClientGUI/ui/sortie/sortie.h
    ClientGUI/ui/sortie/sortie.ui
    ClientGUI/ui/techview.cpp
    ClientGUI/ui/techview.h
    ClientGUI/ui/techview.ui
    ClientGUI/ui/views/equipselect.cpp
    ClientGUI/ui/views/equipselect.h
    ClientGUI/ui/views/equipview.cpp
    ClientGUI/ui/views/equipview.h
    ClientGUI/ui/views/equipview.ui
    ClientGUI/ui/views/hpdelegate.cpp
    ClientGUI/ui/views/hpdelegate.h
    ClientGUI/ui/views/industrialselect.cpp
    ClientGUI/ui/views/industrialselect.h
    ClientGUI/ui/views/selectdelegate.cpp
    ClientGUI/ui/views/selectdelegate.h
    ClientGUI/ui/views/shipselect.cpp
    ClientGUI/ui/views/shipselect.h
    ${TS_FILES}
)
set(SERVER_SOURCES
    Server/erfinv.h
    Server/fleetinfo.cpp
    Server/fleetinfo.h
    Server/kerrors.cpp
    Server/kerrors.h
    Server/main.cpp
    Server/qconsolelistener.cpp
    Server/qconsolelistener.h
    Server/rngesus.cpp
    Server/rngesus.h
    Server/server.cpp
    Server/server.h
    Server/server_ard.cpp
    Server/server_battle.cpp
    Server/server_import.cpp
    Server/server_offer.cpp
    Server/server_sqlinit.cpp
    Server/server_test.cpp
    Server/servermastersender.cpp
    Server/servermastersender.h
    Server/sslserver.cpp
    Server/sslserver.h
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
    Protocol/lua.cpp
    Protocol/lua.h
    Protocol/map.cpp
    Protocol/map.h
    Protocol/mapwithdiff.cpp
    Protocol/mapwithdiff.h
    Protocol/peerinfo.cpp
    Protocol/peerinfo.h
    Protocol/receiver.cpp
    Protocol/receiver.h
    Protocol/resexotic.cpp
    Protocol/resexotic.h
    Protocol/resord.cpp
    Protocol/resord.h
    Protocol/sender.cpp
    Protocol/sender.h
    Protocol/ship.cpp
    Protocol/ship.h
    Protocol/shipdynamic.cpp
    Protocol/shipdynamic.h
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
    set_source_files_properties(${app_icon_resource_windows} PROPERTIES
        COMPILE_FLAGS "/I\"${CMAKE_CURRENT_SOURCE_DIR}/resources\""
    )
endif()

qt_add_library(CFProtocol
    STATIC
    MANUAL_FINALIZATION
    ${PROTOCOL_SOURCES}
    Protocol/mapnode.cpp Protocol/mapnode.h
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

target_include_directories(CFClient PRIVATE ${PROJECT_SOURCE_DIR}/ClientGUI/ui/sortie/opengl/tinygltf)

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
#    set_property(TARGET FleetMemories APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation

#qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})

qt_add_translations(
    TARGETS CFClient CFProtocol CFServer
    TS_FILES ${TS_FILES}
    INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}
    LRELEASE_OPTIONS -idbased)

add_custom_target(copy_assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${PROJECT_SOURCE_DIR}/lua"
    "${PROJECT_BINARY_DIR}/lua"
    COMMENT "Copying lua files"
)
# Ensure lrelease runs after lupdate so .qm files pick up new IDs.
# release_translations is created by qt_add_translations via deferred calls,
# so we use cmake_language(DEFER) to add the dependency after it exists.
cmake_language(DEFER CALL add_dependencies release_translations update_translations)
add_dependencies(CFProtocol update_translations)
add_dependencies(CFClient update_translations)
add_dependencies(CFServer update_translations)
add_dependencies(CFProtocol copy_assets)
add_dependencies(CFClient copy_assets)
add_dependencies(CFServer copy_assets)

target_link_libraries(CFProtocol PRIVATE Qt${QT_VERSION_MAJOR}::Core)
target_link_libraries(CFProtocol PRIVATE Qt${QT_VERSION_MAJOR}::Sql)
target_link_libraries(CFProtocol PRIVATE Qt${QT_VERSION_MAJOR}::Network)
target_link_libraries(CFProtocol PRIVATE lua_library)
target_link_libraries(CFClient PRIVATE Qt${QT_VERSION_MAJOR}::HttpServer)
target_link_libraries(CFClient PRIVATE Qt${QT_VERSION_MAJOR}::SvgWidgets)
target_link_libraries(CFClient PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(CFClient PRIVATE Qt${QT_VERSION_MAJOR}::Network)
target_link_libraries(CFClient PRIVATE Qt${QT_VERSION_MAJOR}::OpenGL)
target_link_libraries(CFClient PRIVATE Qt${QT_VERSION_MAJOR}::OpenGLWidgets)
target_link_libraries(CFClient PRIVATE CFProtocol)
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    target_link_libraries(CFClient PRIVATE factoryslot/FactorySlot)
    target_link_libraries(CFClient PRIVATE repairslot/RepairSlot)
else ()
    target_link_libraries(CFClient PRIVATE ${CMAKE_BINARY_DIR}/FactorySlot/libFactorySlot.so)
    target_link_libraries(CFClient PRIVATE ${CMAKE_BINARY_DIR}/RepairSlot/libRepairSlot.so)
endif()
target_link_libraries(CFClient PRIVATE ${STEAM_LIBRARY})
target_link_libraries(CFClient PRIVATE ${STEAM_APPENCRYPT_LIBRARY})
target_link_libraries(CFClient PRIVATE lua_library)
target_link_libraries(CFServer PRIVATE Qt${QT_VERSION_MAJOR}::Core)
target_link_libraries(CFServer PRIVATE Qt${QT_VERSION_MAJOR}::Sql)
target_link_libraries(CFServer PRIVATE Qt${QT_VERSION_MAJOR}::Network)
target_link_libraries(CFServer PRIVATE CFProtocol)
target_link_libraries(CFServer PRIVATE ${STEAM_LIBRARY})
target_link_libraries(CFServer PRIVATE ${STEAM_APPENCRYPT_LIBRARY})
target_link_libraries(CFServer PRIVATE lua_library)

set(shaders_resource_files
    "ClientGUI/ui/sortie/opengl/fshader.glsl"
    "ClientGUI/ui/sortie/opengl/vshader.glsl"
)

qt6_add_resources(CFClient "shaders"
    PREFIX
    "/"
    FILES
    ${shaders_resource_files}
)

set_target_properties(CFClient PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER moe.fleetmemories.fleetmemories
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
    add_custom_command(TARGET CFClient POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_BINARY_DIR}/RepairSlot/RepairSlot.dll"
        $<TARGET_FILE_DIR:CFClient>)
    add_custom_command(TARGET CFServer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_BINARY_DIR}/FactorySlot/FactorySlot.dll"
        $<TARGET_FILE_DIR:CFServer>)
    add_custom_command(TARGET CFServer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_BINARY_DIR}/RepairSlot/RepairSlot.dll"
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

add_custom_command(TARGET CFServer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${PROJECT_SOURCE_DIR}/../doc/equip/Equip.csv"
    $<TARGET_FILE_DIR:CFServer>)
add_custom_command(TARGET CFServer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${PROJECT_SOURCE_DIR}/../doc/ship/Ship.csv"
    $<TARGET_FILE_DIR:CFServer>)
add_custom_command(TARGET CFServer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${PROJECT_SOURCE_DIR}/../doc/map/Map_nodes.csv"
    $<TARGET_FILE_DIR:CFServer>)
add_custom_command(TARGET CFServer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${PROJECT_SOURCE_DIR}/../doc/map/Map_relations.csv"
    $<TARGET_FILE_DIR:CFServer>)
add_custom_command(TARGET CFClient POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${PROJECT_SOURCE_DIR}/../doc/map/Map_relations.csv"
    $<TARGET_FILE_DIR:CFClient>)
add_custom_command(TARGET CFClient POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${PROJECT_SOURCE_DIR}/../doc/map/Map_nodes.csv"
    $<TARGET_FILE_DIR:CFClient>)
add_custom_command(TARGET CFServer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${PROJECT_SOURCE_DIR}/../doc/equip/Precondition_relations.csv"
    $<TARGET_FILE_DIR:CFServer>)
