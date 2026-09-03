include(CTest)

macro(add_pocket_test name src)
    add_executable(${name} ${src})
    target_include_directories(${name} PRIVATE
        ${CMAKE_SOURCE_DIR}/apps/companion/include
        ${CMAKE_SOURCE_DIR}/apps/desktop/include
    )
    target_link_libraries(${name} PRIVATE
        Qt6::Core
        Qt6::Test
        Qt6::Network
        Qt6::Widgets
        pocket_core
        pocket_storage
        pocket_emulator_api
        pocket_save
        pocket_companion_core
        pocket_input
    )
    add_test(NAME ${name} COMMAND ${name})
    if(WIN32 AND EXISTS "C:/Qt/6.7.2/msvc2019_64/bin")
        set_tests_properties(${name} PROPERTIES ENVIRONMENT "PATH=C:/Qt/6.7.2/msvc2019_64/bin;$ENV{PATH};QT_PLUGIN_PATH=C:/Qt/6.7.2/msvc2019_64/plugins")
    endif()
endmacro()

add_pocket_test(test_game_system test_game_system.cpp)
add_pocket_test(test_path_validation test_path_validation.cpp)
add_pocket_test(test_database_migration test_database_migration.cpp)
add_pocket_test(test_game_identity test_game_identity.cpp)
add_pocket_test(test_duplicate_import test_duplicate_import.cpp)
add_pocket_test(test_companion_link test_companion_link.cpp)
add_pocket_test(test_save_safety test_save_safety.cpp)
add_pocket_test(test_companion_state test_companion_state.cpp)

# Sprite Provider & Cache Tests
add_pocket_test(test_sprite_provider test_sprite_provider.cpp)

# ROM Fingerprint, Game Metadata & Artwork Tests
add_pocket_test(test_game_metadata_artwork test_game_metadata_artwork.cpp)

# IPC & Process Architecture Tests
add_pocket_test(test_ipc_connection test_ipc_connection.cpp)
add_pocket_test(test_ipc_reconnect test_ipc_reconnect.cpp)
add_pocket_test(test_main_app_absent test_main_app_absent.cpp)
add_pocket_test(test_companion_absent test_companion_absent.cpp)
add_pocket_test(test_simultaneous_db_read test_simultaneous_db_read.cpp)
add_pocket_test(test_graceful_shutdown test_graceful_shutdown.cpp)

# Companion Simulation Engine Tests
add_pocket_test(test_companion_simulation test_companion_simulation.cpp)

# Emulator Engine Tests
add_pocket_test(test_mgba_engine test_mgba_engine.cpp)
add_executable(test_melonds_engine
    test_melonds_engine.cpp
    ${CMAKE_SOURCE_DIR}/apps/desktop/include/ControllerArtworkLayer.hpp
    ${CMAKE_SOURCE_DIR}/apps/desktop/src/ControllerArtworkLayer.cpp
    ${CMAKE_SOURCE_DIR}/apps/desktop/include/ControllerHintOverlay.hpp
    ${CMAKE_SOURCE_DIR}/apps/desktop/src/ControllerHintOverlay.cpp
    ${CMAKE_SOURCE_DIR}/apps/desktop/include/NdsDisplayWidget.hpp
    ${CMAKE_SOURCE_DIR}/apps/desktop/include/NdsDisplayTransform.hpp
    ${CMAKE_SOURCE_DIR}/apps/desktop/src/NdsDisplayWidget.cpp
    ${CMAKE_SOURCE_DIR}/apps/desktop/src/NdsDisplayTransform.cpp
)
target_include_directories(test_melonds_engine PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/desktop/include
)
target_link_libraries(test_melonds_engine PRIVATE
    Qt6::Core
    Qt6::Test
    Qt6::Widgets
    Qt6::Svg
    Qt6::SvgWidgets
    pocket_core
    pocket_storage
    pocket_emulator_api
    pocket_save
    pocket_companion_core
    pocket_input
)
add_test(NAME test_melonds_engine COMMAND test_melonds_engine)
if(WIN32 AND EXISTS "C:/Qt/6.7.2/msvc2019_64/bin")
    set_tests_properties(test_melonds_engine PROPERTIES ENVIRONMENT "PATH=C:/Qt/6.7.2/msvc2019_64/bin;$ENV{PATH};QT_PLUGIN_PATH=C:/Qt/6.7.2/msvc2019_64/plugins")
endif()


add_executable(test_nds_display_transform
    test_nds_display_transform.cpp
    ${CMAKE_SOURCE_DIR}/apps/desktop/include/NdsDisplayTransform.hpp
    ${CMAKE_SOURCE_DIR}/apps/desktop/src/NdsDisplayTransform.cpp
)
target_include_directories(test_nds_display_transform PRIVATE ${CMAKE_SOURCE_DIR}/apps/desktop/include)
target_link_libraries(test_nds_display_transform PRIVATE Qt6::Core Qt6::Test)
add_test(NAME test_nds_display_transform COMMAND test_nds_display_transform)
if(WIN32 AND EXISTS "C:/Qt/6.7.2/msvc2019_64/bin")
    set_tests_properties(test_nds_display_transform PROPERTIES ENVIRONMENT "PATH=C:/Qt/6.7.2/msvc2019_64/bin;$ENV{PATH};QT_PLUGIN_PATH=C:/Qt/6.7.2/msvc2019_64/plugins")
endif()

add_pocket_test(test_melonds_engine_real test_melonds_engine_real.cpp)
add_pocket_test(test_gen4_emulation_save_integration test_gen4_emulation_save_integration.cpp)

add_pocket_test(test_controller_layout test_controller_layout.cpp)
target_link_libraries(test_controller_layout PRIVATE Qt6::Svg)
add_pocket_test(test_controller_mapping test_controller_mapping.cpp)

add_executable(test_controller_mapper_widget
    test_controller_mapper_widget.cpp
    ${CMAKE_SOURCE_DIR}/apps/desktop/include/ControllerArtworkLayer.hpp
    ${CMAKE_SOURCE_DIR}/apps/desktop/src/ControllerArtworkLayer.cpp
    ${CMAKE_SOURCE_DIR}/apps/desktop/include/InteractiveControlLayer.hpp
    ${CMAKE_SOURCE_DIR}/apps/desktop/src/InteractiveControlLayer.cpp
    ${CMAKE_SOURCE_DIR}/apps/desktop/include/ControllerMapperWidget.hpp
    ${CMAKE_SOURCE_DIR}/apps/desktop/src/ControllerMapperWidget.cpp
)
target_include_directories(test_controller_mapper_widget PRIVATE ${CMAKE_SOURCE_DIR}/apps/desktop/include)
target_link_libraries(test_controller_mapper_widget PRIVATE Qt6::Core Qt6::Test Qt6::Widgets Qt6::Svg Qt6::SvgWidgets pocket_input)
add_test(NAME test_controller_mapper_widget COMMAND test_controller_mapper_widget)
if(WIN32 AND EXISTS "C:/Qt/6.7.2/msvc2019_64/bin")
    set_tests_properties(test_controller_mapper_widget PROPERTIES ENVIRONMENT "PATH=C:/Qt/6.7.2/msvc2019_64/bin;$ENV{PATH};QT_PLUGIN_PATH=C:/Qt/6.7.2/msvc2019_64/plugins")
endif()

add_executable(test_emulator_input_binding
    test_emulator_input_binding.cpp
    ${CMAKE_SOURCE_DIR}/apps/desktop/include/ControllerArtworkLayer.hpp
    ${CMAKE_SOURCE_DIR}/apps/desktop/src/ControllerArtworkLayer.cpp
    ${CMAKE_SOURCE_DIR}/apps/desktop/include/ControllerHintOverlay.hpp
    ${CMAKE_SOURCE_DIR}/apps/desktop/src/ControllerHintOverlay.cpp
    ${CMAKE_SOURCE_DIR}/apps/desktop/include/EmulatorWidget.hpp
    ${CMAKE_SOURCE_DIR}/apps/desktop/src/EmulatorWidget.cpp
    ${CMAKE_SOURCE_DIR}/apps/desktop/include/AudioSink.hpp
    ${CMAKE_SOURCE_DIR}/apps/desktop/src/AudioSink.cpp
)
target_include_directories(test_emulator_input_binding PRIVATE ${CMAKE_SOURCE_DIR}/apps/desktop/include)
target_link_libraries(test_emulator_input_binding PRIVATE Qt6::Core Qt6::Test Qt6::Widgets Qt6::Svg Qt6::SvgWidgets pocket_input pocket_emulator_api pocket_save pocket_core)
if(WIN32)
    target_link_libraries(test_emulator_input_binding PRIVATE winmm)
endif()

add_executable(test_controller_hint_overlay
    test_controller_hint_overlay.cpp
    ${CMAKE_SOURCE_DIR}/apps/desktop/include/ControllerArtworkLayer.hpp
    ${CMAKE_SOURCE_DIR}/apps/desktop/src/ControllerArtworkLayer.cpp
    ${CMAKE_SOURCE_DIR}/apps/desktop/include/ControllerHintOverlay.hpp
    ${CMAKE_SOURCE_DIR}/apps/desktop/src/ControllerHintOverlay.cpp
    ${CMAKE_SOURCE_DIR}/apps/desktop/include/EmulatorWidget.hpp
    ${CMAKE_SOURCE_DIR}/apps/desktop/src/EmulatorWidget.cpp
    ${CMAKE_SOURCE_DIR}/apps/desktop/include/NdsDisplayWidget.hpp
    ${CMAKE_SOURCE_DIR}/apps/desktop/src/NdsDisplayWidget.cpp
    ${CMAKE_SOURCE_DIR}/apps/desktop/include/NdsDisplayTransform.hpp
    ${CMAKE_SOURCE_DIR}/apps/desktop/src/NdsDisplayTransform.cpp
    ${CMAKE_SOURCE_DIR}/apps/desktop/include/AudioSink.hpp
    ${CMAKE_SOURCE_DIR}/apps/desktop/src/AudioSink.cpp
)
target_include_directories(test_controller_hint_overlay PRIVATE ${CMAKE_SOURCE_DIR}/apps/desktop/include)
target_link_libraries(test_controller_hint_overlay PRIVATE
    Qt6::Core Qt6::Gui Qt6::Test Qt6::Widgets Qt6::Svg Qt6::SvgWidgets
    pocket_input pocket_emulator_api pocket_save pocket_core
)
if(WIN32)
    target_link_libraries(test_controller_hint_overlay PRIVATE winmm)
endif()
add_test(NAME test_controller_hint_overlay COMMAND test_controller_hint_overlay)
if(WIN32 AND EXISTS "C:/Qt/6.7.2/msvc2019_64/bin")
    set_tests_properties(test_controller_hint_overlay PROPERTIES ENVIRONMENT "PATH=C:/Qt/6.7.2/msvc2019_64/bin;$ENV{PATH};QT_PLUGIN_PATH=C:/Qt/6.7.2/msvc2019_64/plugins")
endif()
add_test(NAME test_emulator_input_binding COMMAND test_emulator_input_binding)
if(WIN32 AND EXISTS "C:/Qt/6.7.2/msvc2019_64/bin")
    set_tests_properties(test_emulator_input_binding PROPERTIES ENVIRONMENT "PATH=C:/Qt/6.7.2/msvc2019_64/bin;$ENV{PATH};QT_PLUGIN_PATH=C:/Qt/6.7.2/msvc2019_64/plugins")
endif()

add_executable(test_melonds_core_probe test_melonds_core_probe.cpp)
target_include_directories(test_melonds_core_probe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src/emulator/include)
target_link_libraries(test_melonds_core_probe PRIVATE Qt6::Core Qt6::Test)
add_test(NAME test_melonds_core_probe COMMAND test_melonds_core_probe)
if(WIN32 AND EXISTS "C:/Qt/6.7.2/msvc2019_64/bin")
    set_tests_properties(test_melonds_core_probe PROPERTIES ENVIRONMENT "PATH=C:/Qt/6.7.2/msvc2019_64/bin;$ENV{PATH};QT_PLUGIN_PATH=C:/Qt/6.7.2/msvc2019_64/plugins")
endif()

add_executable(test_controller_artwork test_controller_artwork.cpp)
target_link_libraries(test_controller_artwork PRIVATE Qt6::Core Qt6::Test Qt6::Gui Qt6::Svg pocket_input)
add_test(NAME test_controller_artwork COMMAND test_controller_artwork)
if(WIN32 AND EXISTS "C:/Qt/6.7.2/msvc2019_64/bin")
    set_tests_properties(test_controller_artwork PROPERTIES ENVIRONMENT "PATH=C:/Qt/6.7.2/msvc2019_64/bin;$ENV{PATH};QT_PLUGIN_PATH=C:/Qt/6.7.2/msvc2019_64/plugins")
endif()

# Save Coordination & Stability Tests
add_pocket_test(test_save_coordination test_save_coordination.cpp)

# Gen 1 - Gen 5 Save Parser & Editor Tests
add_pocket_test(test_gen1_gen2_save_parser test_gen1_gen2_save_parser.cpp)
add_pocket_test(test_gen3_save_parser test_gen3_save_parser.cpp)
add_pocket_test(test_gen4_gen5_save_parser test_gen4_gen5_save_parser.cpp)
add_pocket_test(test_gen3_save_editor test_gen3_save_editor.cpp)
add_pocket_test(test_gen3_ev_mutation test_gen3_ev_mutation.cpp)

# External Save Watcher Tests
add_pocket_test(test_external_save_watcher test_external_save_watcher.cpp)

# Companion Re-identification Tests
add_pocket_test(test_companion_reidentification test_companion_reidentification.cpp)

# Training Ledger & Anti-Abuse Rules Tests
add_pocket_test(test_training_ledger test_training_ledger.cpp)

# Framerate Policy Tests
add_executable(test_framerate_policy
    test_framerate_policy.cpp
    ${CMAKE_SOURCE_DIR}/apps/companion/include/CompanionAnimationController.hpp
    ${CMAKE_SOURCE_DIR}/apps/companion/src/CompanionAnimationController.cpp
)
target_include_directories(test_framerate_policy PRIVATE ${CMAKE_SOURCE_DIR}/apps/companion/include)
target_link_libraries(test_framerate_policy PRIVATE Qt6::Core Qt6::Test Qt6::Widgets)
add_test(NAME test_framerate_policy COMMAND test_framerate_policy)
if(WIN32 AND EXISTS "C:/Qt/6.7.2/msvc2019_64/bin")
    set_tests_properties(test_framerate_policy PROPERTIES ENVIRONMENT "PATH=C:/Qt/6.7.2/msvc2019_64/bin;$ENV{PATH};QT_PLUGIN_PATH=C:/Qt/6.7.2/msvc2019_64/plugins")
endif()

# Widget Performance & Animation Policy Tests
add_executable(test_companion_widget_performance
    test_companion_widget_performance.cpp
    ${CMAKE_SOURCE_DIR}/apps/companion/include/CompanionAnimationController.hpp
    ${CMAKE_SOURCE_DIR}/apps/companion/src/CompanionAnimationController.cpp
    ${CMAKE_SOURCE_DIR}/apps/companion/include/PowerStatusMonitor.hpp
    ${CMAKE_SOURCE_DIR}/apps/companion/src/PowerStatusMonitor.cpp
    ${CMAKE_SOURCE_DIR}/apps/companion/include/CompanionStartupManager.hpp
    ${CMAKE_SOURCE_DIR}/apps/companion/src/CompanionStartupManager.cpp
)
target_include_directories(test_companion_widget_performance PRIVATE ${CMAKE_SOURCE_DIR}/apps/companion/include)
target_link_libraries(test_companion_widget_performance PRIVATE Qt6::Core Qt6::Test Qt6::Widgets)
add_test(NAME test_companion_widget_performance COMMAND test_companion_widget_performance)
if(WIN32 AND EXISTS "C:/Qt/6.7.2/msvc2019_64/bin")
    set_tests_properties(test_companion_widget_performance PROPERTIES ENVIRONMENT "PATH=C:/Qt/6.7.2/msvc2019_64/bin;$ENV{PATH};QT_PLUGIN_PATH=C:/Qt/6.7.2/msvc2019_64/plugins")
endif()

add_pocket_test(test_libretro_core_session test_libretro_core_session.cpp)
add_pocket_test(test_libretro_core_probe test_libretro_core_probe.cpp)

add_executable(test_cores_widget
    test_cores_widget.cpp
    ${CMAKE_SOURCE_DIR}/apps/desktop/include/CoresWidget.hpp
    ${CMAKE_SOURCE_DIR}/apps/desktop/src/CoresWidget.cpp
)
target_include_directories(test_cores_widget PRIVATE ${CMAKE_SOURCE_DIR}/apps/desktop/include)
target_link_libraries(test_cores_widget PRIVATE Qt6::Core Qt6::Test Qt6::Widgets pocket_core pocket_emulator_api)
add_test(NAME test_cores_widget COMMAND test_cores_widget)
if(WIN32 AND EXISTS "C:/Qt/6.7.2/msvc2019_64/bin")
    set_tests_properties(test_cores_widget PROPERTIES ENVIRONMENT "PATH=C:/Qt/6.7.2/msvc2019_64/bin;$ENV{PATH};QT_PLUGIN_PATH=C:/Qt/6.7.2/msvc2019_64/plugins")
endif()
add_pocket_test(test_audio_ring_buffer test_audio_ring_buffer.cpp)

add_executable(test_audio_sink
    test_audio_sink.cpp
    ${CMAKE_SOURCE_DIR}/apps/desktop/include/AudioSink.hpp
    ${CMAKE_SOURCE_DIR}/apps/desktop/src/AudioSink.cpp
)
target_include_directories(test_audio_sink PRIVATE ${CMAKE_SOURCE_DIR}/apps/desktop/include)
target_link_libraries(test_audio_sink PRIVATE Qt6::Core Qt6::Test pocket_emulator_api)
if(WIN32)
    target_link_libraries(test_audio_sink PRIVATE winmm)
endif()
add_test(NAME test_audio_sink COMMAND test_audio_sink)
if(WIN32 AND EXISTS "C:/Qt/6.7.2/msvc2019_64/bin")
    set_tests_properties(test_audio_sink PROPERTIES ENVIRONMENT "PATH=C:/Qt/6.7.2/msvc2019_64/bin;$ENV{PATH};QT_PLUGIN_PATH=C:/Qt/6.7.2/msvc2019_64/plugins")
endif()

add_executable(test_emulator_controls
    test_emulator_controls.cpp
    ${CMAKE_SOURCE_DIR}/apps/desktop/include/AudioSink.hpp
    ${CMAKE_SOURCE_DIR}/apps/desktop/src/AudioSink.cpp
)
target_include_directories(test_emulator_controls PRIVATE ${CMAKE_SOURCE_DIR}/apps/desktop/include)
target_link_libraries(test_emulator_controls PRIVATE Qt6::Core Qt6::Test pocket_emulator_api)
if(WIN32)
    target_link_libraries(test_emulator_controls PRIVATE winmm)
endif()
add_test(NAME test_emulator_controls COMMAND test_emulator_controls)
if(WIN32 AND EXISTS "C:/Qt/6.7.2/msvc2019_64/bin")
    set_tests_properties(test_emulator_controls PROPERTIES ENVIRONMENT "PATH=C:/Qt/6.7.2/msvc2019_64/bin;$ENV{PATH};QT_PLUGIN_PATH=C:/Qt/6.7.2/msvc2019_64/plugins")
endif()

add_pocket_test(test_rom_fingerprint_streaming test_rom_fingerprint_streaming.cpp)
