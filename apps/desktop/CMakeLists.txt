add_executable(PocketPartner
    include/LibraryWidget.hpp
    include/CompanionWidget.hpp
    include/SettingsWidget.hpp
    include/MainWindow.hpp
    src/LibraryWidget.cpp
    src/CompanionWidget.cpp
    src/SettingsWidget.cpp
    src/MainWindow.cpp
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
    pocket_core
    pocket_storage
    pocket_emulator_api
    pocket_save
    pocket_companion_core
    PocketPartnerStorage
)
