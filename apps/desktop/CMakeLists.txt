add_executable(PocketPartner
    include/MainWindow.hpp
    include/LibraryWidget.hpp
    include/SettingsWidget.hpp
    include/EmulatorWidget.hpp
    include/DiagnosticsWidget.hpp
    include/TrainingTimingBarWidget.hpp
    include/NdsDisplayWidget.hpp
    include/CompanionWidget.hpp
    include/ControllerArtworkLayer.hpp
    include/InteractiveControlLayer.hpp
    include/ControllerMapperWidget.hpp
    src/main.cpp
    src/MainWindow.cpp
    src/LibraryWidget.cpp
    src/SettingsWidget.cpp
    src/EmulatorWidget.cpp
    src/DiagnosticsWidget.cpp
    src/TrainingTimingBarWidget.cpp
    src/NdsDisplayWidget.cpp
    src/CompanionWidget.cpp
    src/ControllerArtworkLayer.cpp
    src/InteractiveControlLayer.cpp
    src/ControllerMapperWidget.cpp
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
    Qt6::Svg
    Qt6::SvgWidgets
    pocket_core
    pocket_storage
    pocket_emulator_api
    pocket_save
    pocket_companion_core
    pocket_input
)

if(WIN32)
    target_link_libraries(PocketPartner PRIVATE dwmapi winmm)
endif()
