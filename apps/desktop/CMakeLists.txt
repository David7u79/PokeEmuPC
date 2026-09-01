add_executable(PocketPartner
    include/MainWindow.hpp
    include/LibraryWidget.hpp
    include/SettingsWidget.hpp
    include/DiagnosticsWidget.hpp
    include/TrainingTimingBarWidget.hpp
    include/NdsDisplayWidget.hpp
    include/CompanionWidget.hpp
    src/main.cpp
    src/MainWindow.cpp
    src/LibraryWidget.cpp
    src/SettingsWidget.cpp
    src/DiagnosticsWidget.cpp
    src/TrainingTimingBarWidget.cpp
    src/NdsDisplayWidget.cpp
    src/CompanionWidget.cpp
)

target_include_directories(PocketPartner PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/core/include
    ${CMAKE_SOURCE_DIR}/src/storage/include
    ${CMAKE_SOURCE_DIR}/src/emulator/include
    ${CMAKE_SOURCE_DIR}/src/save/include
    ${CMAKE_SOURCE_DIR}/src/companion/include
)

target_link_libraries(PocketPartner PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Sql
    pocket_core
    pocket_storage
    pocket_emulator_api
    pocket_save
    pocket_companion_core
)

if(WIN32)
    target_link_libraries(PocketPartner PRIVATE dwmapi)
endif()
