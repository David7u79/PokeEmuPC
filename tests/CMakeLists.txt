include(CTest)

macro(add_pocket_test name src)
    add_executable(${name} ${src})
    target_link_libraries(${name} PRIVATE
        Qt6::Core
        Qt6::Test
        pocket_core
        pocket_storage
        pocket_emulator_api
        pocket_save
        pocket_companion_core
        PocketPartnerDesktopCompanion
    )
    add_test(NAME ${name} COMMAND ${name})
endmacro()

add_pocket_test(test_game_system test_game_system.cpp)
add_pocket_test(test_path_validation test_path_validation.cpp)
add_pocket_test(test_database_migration test_database_migration.cpp)
add_pocket_test(test_game_identity test_game_identity.cpp)
add_pocket_test(test_duplicate_import test_duplicate_import.cpp)
add_pocket_test(test_companion_link test_companion_link.cpp)
add_pocket_test(test_save_safety test_save_safety.cpp)
add_pocket_test(test_companion_state test_companion_state.cpp)
add_pocket_test(test_framerate_policy test_framerate_policy.cpp)
