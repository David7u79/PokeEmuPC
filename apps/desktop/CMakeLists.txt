add_executable(PocketPartner
    include/MainWindow.hpp
    include/LibraryWidget.hpp
    include/CompanionWidget.hpp
    include/SettingsWidget.hpp
    include/EmulatorWidget.hpp
    src/MainWindow.cpp
    src/LibraryWidget.cpp
    src/CompanionWidget.cpp
    src/SettingsWidget.cpp
    src/EmulatorWidget.cpp
    src/main.cpp
)

target_include_directories(PocketPartner PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(PocketPartner PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Sql
    Qt6::Network
    pocket_core
    pocket_storage
    pocket_emulator_api
    pocket_save
    pocket_companion_core
)

if(MSVC)
    target_link_libraries(PocketPartner PRIVATE winmm)
endif()
