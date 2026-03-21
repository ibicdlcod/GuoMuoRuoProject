message("-- Linking repairSlot")
qt_add_plugin(RepairSlot
)
target_sources(RepairSlot PUBLIC
    repairslot.cpp
    repairslot.h
    repairslotplugin.cpp
    repairslotplugin.h
)
target_link_libraries(RepairSlot PUBLIC Qt${QT_VERSION_MAJOR}::Core)
target_link_libraries(RepairSlot PUBLIC Qt${QT_VERSION_MAJOR}::Gui)
target_link_libraries(RepairSlot PUBLIC Qt${QT_VERSION_MAJOR}::UiPlugin)
target_link_libraries(RepairSlot PUBLIC Qt${QT_VERSION_MAJOR}::Widgets)
set_target_properties(RepairSlot PROPERTIES LINKER_LANGUAGE CXX)
