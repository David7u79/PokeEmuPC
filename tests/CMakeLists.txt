include(CTest)

macro(add_pocket_test name src)
    add_executable(${name} ${src})
    target_include_directories(${name} PRIVATE
        ${CMAKE_SOURCE_DIR}/apps/companion/include
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

# Save Coordination & Stability Tests
add_pocket_test(test_save_coordination test_save_coordination.cpp)

# Gen 3 Save Parser Tests
add_pocket_test(test_gen3_save_parser test_gen3_save_parser.cpp)

# Companion Re-identification Tests
add_pocket_test(test_companion_reidentification test_companion_reidentification.cpp)

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
