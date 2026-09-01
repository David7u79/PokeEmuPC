add_executable(PocketCompanion
    include/DesktopWidget.hpp
    include/CompanionTrayIcon.hpp
    include/CompanionAnimationController.hpp
    include/PowerStatusMonitor.hpp
    include/CompanionStartupManager.hpp
    src/DesktopWidget.cpp
    src/CompanionTrayIcon.cpp
    src/CompanionAnimationController.cpp
    src/PowerStatusMonitor.cpp
    src/CompanionStartupManager.cpp
    src/main.cpp
)

target_include_directories(PocketCompanion PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/core/include
    ${CMAKE_SOURCE_DIR}/src/companion/include
    ${CMAKE_SOURCE_DIR}/src/storage/include
    ${CMAKE_SOURCE_DIR}/src/save/include
)

target_link_libraries(PocketCompanion PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    pocket_core
    pocket_companion_core
    pocket_storage
    pocket_save
)
