add_library(PocketPartnerDesktopCompanion STATIC
    include/pocketpartner/desktop_companion/FramerateGovernor.hpp
    src/FramerateGovernor.cpp
)

target_include_directories(PocketPartnerDesktopCompanion PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(PocketPartnerDesktopCompanion PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    PocketPartnerCore
    PocketPartnerCompanion
    PocketPartnerStorage
)
