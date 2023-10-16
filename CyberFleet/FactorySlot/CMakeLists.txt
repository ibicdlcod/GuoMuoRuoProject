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
