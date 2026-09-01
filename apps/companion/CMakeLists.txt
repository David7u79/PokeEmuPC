add_executable(PocketCompanion
    include/DesktopWidget.hpp
    include/CompanionTrayIcon.hpp
    src/DesktopWidget.cpp
    src/CompanionTrayIcon.cpp
    src/main.cpp
)

target_include_directories(PocketCompanion PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(PocketCompanion PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    pocket_core
    pocket_companion_core
    PocketPartnerDesktopCompanion
)
