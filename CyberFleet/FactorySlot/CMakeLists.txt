message("-- Linking FactorySlot --")

cmake_minimum_required(VERSION 3.16)
project(FactorySlot LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui UiPlugin Widgets)

qt_add_plugin(FactorySlot
    STATIC
)

target_sources(FactorySlot PUBLIC
    factoryslot.cpp
    factoryslot.h
    factoryslotplugin.cpp
    factoryslotplugin.h
)

target_link_libraries(FactorySlot PUBLIC
    Qt::Core
    Qt::Gui
    Qt::UiPlugin
    Qt::Widgets
)
set_target_properties(FactorySlot PROPERTIES LINKER_LANGUAGE CXX)

set(INSTALL_EXAMPLEDIR "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_PLUGINS}/designer")
install(TARGETS FactorySlot
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
