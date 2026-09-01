include(CTest)

macro(add_pocketpartner_test name src)
    add_executable(${name} ${src})
    target_link_libraries(${name} PRIVATE
        Qt6::Core
        Qt6::Test
        PocketPartnerCore
        PocketPartnerEmulator
        PocketPartnerSave
        PocketPartnerCompanion
        PocketPartnerStorage
        PocketPartnerDesktopCompanion
    )
    add_test(NAME ${name} COMMAND ${name})
endmacro()

add_pocketpartner_test(test_companion_link test_companion_link.cpp)
add_pocketpartner_test(test_save_safety test_save_safety.cpp)
add_pocketpartner_test(test_companion_state test_companion_state.cpp)
add_pocketpartner_test(test_framerate_policy test_framerate_policy.cpp)
