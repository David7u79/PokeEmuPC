add_executable(PocketPartner
    src/main.cpp
)

target_link_libraries(PocketPartner PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    PocketPartnerCore
    PocketPartnerEmulator
    PocketPartnerSave
    PocketPartnerCompanion
    PocketPartnerStorage
)
