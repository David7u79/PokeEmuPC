add_executable(PocketCompanion
    src/main.cpp
)

target_link_libraries(PocketCompanion PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    pocket_core
    pocket_companion_core
    PocketPartnerDesktopCompanion
)
