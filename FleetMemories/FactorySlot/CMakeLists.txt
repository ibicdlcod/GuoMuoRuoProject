#message("-- Linking FactorySlot --")
#
#cmake_minimum_required(VERSION 3.16)
#project(FactorySlot LANGUAGES CXX)
#
#find_package(Qt6 REQUIRED COMPONENTS Core Gui UiPlugin Widgets)
#
message("-- Linking FactorySlot")
qt_add_plugin(FactorySlot
)
target_sources(FactorySlot PUBLIC
    factoryslot.cpp
    factoryslot.h
    factoryslotplugin.cpp
    factoryslotplugin.h
)
target_link_libraries(FactorySlot PUBLIC Qt${QT_VERSION_MAJOR}::Core)
target_link_libraries(FactorySlot PUBLIC Qt${QT_VERSION_MAJOR}::Gui)
target_link_libraries(FactorySlot PUBLIC Qt${QT_VERSION_MAJOR}::UiPlugin)
target_link_libraries(FactorySlot PUBLIC Qt${QT_VERSION_MAJOR}::Widgets)
set_target_properties(FactorySlot PROPERTIES LINKER_LANGUAGE CXX)
